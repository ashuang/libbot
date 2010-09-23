#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

//#include <pthread.h>
#include <glib.h>
#include <stdint.h>
#include <assert.h>

#include <bot_core/bot_core.h>

#ifndef PI
#define PI 3.14159265358979323
#endif

/** What is the maximum rotational period of the sick?  If the number
    is too high, performance of synchronizer will decrease
    (modestly). If the number is too small, the synchronized time
    estimate can drift arbitrarily far behind true time. 

    Given a nominal 75Hz rotation, we suggest a value corresponding to
    70Hz (14.3ms).
**/

// the true rotational speed (nominal)
#define SICK_ROTATIONAL_PERIOD_NS ((int) 13.3 * 1000000)

// how much do we add to the true speed to obtain an upper bound?
#define SICK_ROTATIONAL_FUDGE_NS  ((int)  1.0 * 1000000)

#define SICK_MAX_MSG_LENGTH 1024

// sick guarantees 14ms, but we have to deal with the possibility that
// we might be context swapped out... so we inflate this number.

// once we start receiving a packet, what is the maximum amount of
// time that can elapse between two bytes?
#define SICK_RX_TIMEOUT_MS 100

// how many times do we retry transactions (before returning an error message?)
#define SICK_OP_MAX_RETRIES 4
// standard response timeout
#define SICK_OP_TIMEOUT_MS 250
#define SICK_CHANGEMODE_TIMEOUT_MS 3500

#include "sick.h"

void sick_handle_scan(sick_t *s, uint8_t *packet);

// fill in a telegram requesting a baud rate change (but doesn't send it)
void sick_make_baud_request(uint8_t *request, int baud);

// fill in a telegram requesting a continuous mode change. (but doesn't send it)
void sick_make_continuous_request(sick_t *s, uint8_t *request, int enable);

// Our driver needs the sick configured in a particular way. This reconfigures the scanner.
int sick_config_write_our_settings(sick_t *s);

// Try to automatically detect the Sick's baud rate.
int sick_detect_baud(sick_t *s, int baudhint);

// Read the sick's configuration
int sick_config_read(sick_t *s);

// utility function: display a packet on stdout
void sick_display_telegram(unsigned char *b);


static int payload_length(uint8_t *data)
{
    return data[2] + (data[3]<<8);
}

static int packet_length(uint8_t *data)
{
    return payload_length(data) + 6;
}

//////////////////////////////////////////////////////////////////////////
struct sick_serial_context
{
  int fd;
};

static void *default_serial_open(const char *port)
{
  struct sick_serial_context *context;
  int fd = bot_serial_open(port, 9600, 1);

  if (fd < 0)
    return NULL;

  context = (struct sick_serial_context*) malloc(sizeof(struct sick_serial_context));
  context->fd = fd;

  return context;
}

static int default_serial_getfd(void *ptr)
{
  struct sick_serial_context *context = (struct sick_serial_context*) ptr;
  return context->fd;
}

static int default_serial_setbaud(void *ptr, int baud)
{  
  struct sick_serial_context *context = (struct sick_serial_context*) ptr;

  return bot_serial_setbaud(context->fd, baud);
}

static int default_serial_close(void *ptr)
{
  struct sick_serial_context *context = (struct sick_serial_context*) ptr;
  close(context->fd);
  free(ptr);
  return 0;
}

//////////////////////////////////////////////////////////////////////////

static int sick_compute_checksum(uint8_t *data)
{
    int payloadlen = payload_length(data);
    int crc = 0;
    int ab = 0;

    for (int i = 0; i < payloadlen + 4; i++) {
        ab <<= 8;
        ab |= data[i];
        
        crc <<= 1;
        if (crc&0x10000)
            crc ^= 0x8005;
        
        crc ^= ab;
    }

    return crc & 0xffff;
}

static void make_telegram(uint8_t *payload, int payloadlen, uint8_t *data)
{
    data[0] = 0x02;
    data[1] = 0x00;
    data[2] = payloadlen&0xff;
    data[3] = payloadlen>>8;

    for (int i = 0; i < payloadlen; i++)
        data[4+i] = payload[i];

    int chk = sick_compute_checksum(data);
    data[4+payloadlen] = chk&0xff;
    data[5+payloadlen] = chk>>8;

//  sick_display_telegram(data);
}

int rxlen = 0;
static int sick_read_packet(sick_t *s, uint8_t *data)
{
    int res;
    int datalen = 0; // how many bytes of data[] are valid?
    int chk, chk2;
    int want;

readmore:
    //    printf("%i\n", rxlen);
    want = 4 - datalen;
    if (want > 0) {
        // we're willing to wait forever for these bytes
        // (this prevents us from spinning in a poll loop
        res = bot_fileutils_read_fully_timeout(s->sick_serialfd, &data[datalen], want, -1);
	rxlen += res;
        if (res <= 0)
            return -1;
        datalen += want;
    }

    // two cases: either the 4 bytes consistute a good header, or
    // we skip along to the next occurence of an STX and try
    // again.

    // is this header good?
    int payloadlen = payload_length(data);

    if (data[0] != 0x02   ||   data[1] != 0x80   ||   payloadlen >= (SICK_MAX_MSG_LENGTH - 6)) {
        goto resync;
    }

    // this header is good. read the message (plus checksum bytes)
    want = payloadlen + 6 - datalen;
    if (want > 0) {
        res = bot_fileutils_read_fully_timeout(s->sick_serialfd, &data[datalen], want, SICK_RX_TIMEOUT_MS);
	rxlen+=res;
        if (res <= 0)
            return -2;
        datalen += want;
    }

    // is the checksum good?
    chk = data[4+payloadlen] + (data[5+payloadlen]<<8);
    chk2 = sick_compute_checksum(data);
    if (chk != chk2) {
        printf("bad chk: %04X != %04X\n", chk, chk2);
        goto resync;
    }
    
    // good packet received!

    // is this the response to a request?
    //pthread_mutex_lock(&s->writelock);
    g_mutex_lock(s->writelock);

    if (s->writedata != NULL && data[4] == s->writereqid) {
        memcpy(s->writedata, data, packet_length(data));
        s->writevalid = 1;
        //pthread_cond_signal(&s->writecond); // signal the sick_transaction function that may be waiting
        g_cond_signal(s->writecond); // signal the sick_transaction function that may be waiting
    } 
    //pthread_mutex_unlock(&s->writelock);
    g_mutex_unlock(s->writelock);

    // is it a laser scan?
    if (data[4] == 0xb0) {
        sick_handle_scan(s, data);
    }

    return 0;

resync:
    for (int i = 1; i < datalen; i++) {
        if (data[i] == 0x02) {
            memmove(data, &data[i], datalen - i);
            datalen = datalen - i;
            goto readmore;
        } 
    }

    // no STX found, start from scratch
    datalen = 0;
    goto readmore;
}

static void *sick_read_thread(void *_arg)
{
    sick_t *s = (sick_t*) _arg;
    uint8_t data[SICK_MAX_MSG_LENGTH];

    while (1) {
        int res = sick_read_packet(s, data);

        if (res < 0) {
            printf("read err %i\n", res);
	    usleep(200000); // sleep for a while, hope the problem goes away
	}
    }
}

static int sick_transaction(sick_t *s, uint8_t *request, uint8_t responsetype, uint8_t *response, int timeoutms)
{
    //pthread_mutex_lock(&s->writelock);
    g_mutex_lock(s->writelock);

    int res = 0;
    int trials = 0;
    int requestlen;

retry:
    requestlen = packet_length(request);
    
    // send the request
    res = bot_fileutils_write_fully(s->sick_serialfd, request, requestlen);

/*
    printf("packet: ");
    for (int i = 0; i < requestlen; i++)
      printf("%02x ", request[i]);
    printf("\n");
*/

    if (res < 0) {
        printf("write failed\n");
        goto exit;
    }

    // wait for response
    s->writereqid = responsetype;
    s->writedata = response;
    s->writevalid = 0;

    //struct timespec ts;
    //timespec_now(&ts);
    //timespec_addms(&ts, timeoutms);

    
    //struct timeval tv;
    GTimeVal tv;
    int sec = timeoutms/1000;
    long ms = timeoutms - sec*1000;
    //gettimeofday (&tv, NULL);
    g_get_current_time(&tv);

    tv.tv_usec += ms*1000;
    tv.tv_sec += tv.tv_usec/1000000 + sec;
    tv.tv_usec = tv.tv_usec%1000000;


    //pthread_cond_timedwait(&s->writecond, &s->writelock, &ts);
    g_cond_timed_wait(s->writecond, s->writelock, &tv);


    if (!s->writevalid) {
        trials++;
        if (trials > SICK_OP_MAX_RETRIES) {
            res = -2;
            goto exit;
        }
        goto retry;
    }

exit:
    s->writedata = NULL;
    s->writereqid = 0;

    //pthread_mutex_unlock(&s->writelock);
    g_mutex_unlock(s->writelock);

    return res;
}

sick_t *sick_create(struct sick_serial_ops *ops)
{
    sick_t *s;

    s=(sick_t*) calloc(sizeof(sick_t),1);
    s->params = (uint8_t*) calloc(SICK_PARAMS_LENGTH, 1);

    if (ops != NULL) {
      memcpy(&s->ops, ops, sizeof(struct sick_serial_ops));
    } else {
      s->ops.sick_serial_open = default_serial_open;
      s->ops.sick_serial_setbaud = default_serial_setbaud;
      s->ops.sick_serial_getfd = default_serial_getfd;
      s->ops.sick_serial_close = default_serial_close;
    }


    //pthread_mutexattr_t mutexAttr;
    //pthread_mutexattr_init(&mutexAttr);
    //pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);

    //pthread_mutex_init(&s->writelock, &mutexAttr);
    //pthread_cond_init(&s->writecond, NULL);

    // create the mutex and condition variable
    s->writelock = g_mutex_new();
    s->writecond = g_cond_new();

    s->writedata = NULL;
    s->callbackranges = NULL;

    s->scan_callback = (sick_scan_callback_t) NULL;

    s->fov_degrees = 0;
    s->res_cdegrees = 0;

    s->sync = bot_timestamp_sync_init(75.0, 256, 76.0/75.0);

    return s;
}

void sick_set_scan_callback(sick_t *s, sick_scan_callback_t callback, void *user)
{
    s->scan_callback = callback;
    s->scan_callback_user = user;
}

void sick_destroy(sick_t *s)
{
    close(s->sick_serialfd);
    free(s->params);
    //pthread_mutex_destroy(&s->writelock);
    //pthread_cond_destroy(&s->writecond);
    g_mutex_free(s->writelock);
    g_cond_free(s->writecond);
    free(s);
}

int sick_connect(sick_t *s, char *port, int baudhint, int fov_degrees, int resolution, int interlaced, int max_range_meters)
{
    int res;

    // verify valid measurement range setting
    if (max_range_meters!=8 && max_range_meters!=16 && 
        max_range_meters!=32 && max_range_meters!=80) {
        fprintf (stderr, "Invalid measurement range = %d. Must be one of 8m, 16m, 32m, or 80m\n", max_range_meters);
        return -1;
    }

    // the mode is okay.
    s->sick_serial_context = s->ops.sick_serial_open(port);
    if (s->sick_serial_context == NULL) {
        fprintf (stderr, "Failed to open %s\n", port);
        return -1;
    }

    if (s->ops.sick_serial_setbaud(s->sick_serial_context, 9600)) {
      fprintf(stderr, "Couldn't initialize serial port to 9600 baud");
      return -1;
    }

    s->sick_serialfd = s->ops.sick_serial_getfd(s->sick_serial_context);

    // Create the read thread
    GThread *newthread;
    newthread = g_thread_create(sick_read_thread, s, TRUE, NULL);
    //pthread_t newthread;
    //pthread_create(&newthread, NULL, sick_read_thread, s);

    // only returns after successful detection
    sick_detect_baud(s, baudhint);

    // setup the sick. (no EEPROM update will occur if the settings are already set.)
    // this also reads the sick configuration.
    s->max_range_meters = max_range_meters;
    res = sick_config_write_our_settings(s);
    if (res < 0)
        return res;

    // make sure we're in mode 0x25 (stop continuous data output).
    res = sick_set_continuous(s, 0);

    res = sick_set_variant(s, fov_degrees, resolution, interlaced);
    if (res < 0)
        return res;

    // turn off continuous mode (which also ensures that s->continuousmode is correct)
    res = sick_set_continuous(s, 0);
    if (res < 0)
        return res;

    return 0;
}

int sick_detect_baud(sick_t *s, int baudhint)
{
    int bauds[]={0, 9600, 19200, 38400, 500000, -1};
    char typebuf[100];
    uint8_t statbuf[512];

    bauds[0] = baudhint;

    while (1) {
        for (int idx = 0; bauds[idx] > 0; idx++) {
            int baud = bauds[idx];
            if (baud==0)
                continue;

            if (baud==baudhint && idx!=0)
                continue;

            printf("Trying %6i baud ...\n", baud);
            fflush(NULL);

	    s->ops.sick_serial_setbaud(s->sick_serial_context, baud);

            int res = sick_get_status(s, statbuf, 512);

            if (!res) {
                // we conditionally perform a second
                // query. this helps us ensure that we
                // only report success for the correct
                // baud rate.  otherwise, we could be
                // receiving responses for requests we
                // received at previous baud rates.
                res = sick_get_type(s, typebuf, 100);
            }

            if (!res) {
                printf("found %s\n", typebuf);
                return baud;
            } 
        }
    }
}

void sick_make_baud_request(uint8_t *request, int baud)
{
    uint8_t payload[2];

    payload[0] = 0x20;
    switch (baud)
    {
    default:
        printf("WARNING: Unsupported baud rate %i. Ignoring.\n", baud);
    case 9600:
        payload[1]=0x42;
        break;
    case 19200:
        payload[1]=0x41;
        break;
    case 38400:
        payload[1]=0x40;
        break;
    case 500000:
//      payload[1]=0x40; // 38.4 actually
        payload[1]=0x48;
        break;
    }
    make_telegram(payload, 2, request); 
}

int sick_set_baud(sick_t *s, int baud)
{
    uint8_t request[SICK_MAX_MSG_LENGTH], response[SICK_MAX_MSG_LENGTH];
    sick_make_baud_request(request, baud);

    int res = sick_transaction(s, request, request[4] | 0x80, response, SICK_OP_TIMEOUT_MS);
    if (res < 0)
        return res;
             
    if (response[5]!=0x00) // success?
        return -1;

    // hurrah!
    s->ops.sick_serial_setbaud(s->sick_serial_context, baud);

    printf("Baud rate changed to %i\n",baud);
    return 0;
}

void sick_display_telegram(unsigned char *b)
{
    if (b[0]!=0x02) {
        printf("invalid telegram\n");
        return;
    }

    if (b[4]==0x92) {
        printf("NACK/Incorrect command\n");
    }

    int packetlength = packet_length(b);

    for (int i = 0; i < packetlength; i++) {
        if ((i%16)==0)
            printf("%04X : ",i);
        printf("%02X ", b[i]);
        if ((i%16)==15)
            printf("\n");
    }
    printf("\n");
}

int sick_request_scan(sick_t *s)
{
    // no need to request scan if we're in continuous mode.
    if (s->continuousmode)
        return 0;

    uint8_t payload[2], request[SICK_MAX_MSG_LENGTH], response[SICK_MAX_MSG_LENGTH];
    payload[0] = 0x30;

    // request interlaced data if higher-angular resolution modes have been requested
    if (s->res_cdegrees != 100)
        payload[1] = 0x08;  // interlaced
    else
        payload[1] = 0x01;  // non-interlaced

    make_telegram(payload, 2, request);

    int res = sick_transaction(s, request, payload[0] | 0x80, response, SICK_OP_TIMEOUT_MS);
    if (res < 0)
        return res;

    return 0;
}

int sick_get_type(sick_t *s, char *buf, int bufmax)
{
    uint8_t payload[1], request[SICK_MAX_MSG_LENGTH], response[SICK_MAX_MSG_LENGTH];
    payload[0] = 0x3a;
    make_telegram(payload, 1, request);

    int res = sick_transaction(s, request, payload[0] | 0x80, response, SICK_OP_TIMEOUT_MS);
    if (res < 0)
        return res;

    int typelen = packet_length(response) - 10;
    if (typelen > bufmax-1)
        typelen = bufmax - 1;
    memcpy(buf, &response[5], typelen);
    buf[typelen] = 0;

    return 0;
}

int sick_get_status(sick_t *s, uint8_t *statbuf, int statbufmax)
{
    uint8_t payload[1], request[SICK_MAX_MSG_LENGTH], response[SICK_MAX_MSG_LENGTH];
    payload[0] = 0x31;
    make_telegram(payload, 1, request);

    int res = sick_transaction(s, request, payload[0] | 0x80, response, SICK_OP_TIMEOUT_MS);
    if (res < 0)
        return res;
    
    int length = payload_length(response);
    if (length > statbufmax)
        length = statbufmax;

    memcpy(statbuf, &response[5], length);

    // huh! The telegram doesn't seem to match the documentation!
//  sick_display_telegram(response);

    return 0;
}

/*
  int sick_get_revolutions(sick_t *s, int *revs)
  {
  uint8_t status[512];

  int res = sick_get_status(s, status, 512);
  if (res < 0)
  return res;

  *revs = status[21] + status[22]<<8;
  return 0;
  }
*/
int sick_config_read(sick_t *s)
{
    uint8_t payload[1], request[SICK_MAX_MSG_LENGTH], response[SICK_MAX_MSG_LENGTH];
    payload[0] = 0x74;
    make_telegram(payload, 1, request);

    int res = sick_transaction(s, request, payload[0] | 0x80, response, SICK_OP_TIMEOUT_MS);
    if (res < 0)
        return res;

    // we read the configuration, yay.
    // make a copy.
    memcpy(s->params, &response[5], SICK_PARAMS_LENGTH);

    int ranges[]={8, 8, 8, 16, 16, 32, 32};

    if (s->params[6]==0x01) {
        s->rangescale = 0.001; // mm resolution
        printf("1 unit = 1mm, range = %im\n", ranges[s->params[5]]);
    } else if (s->params[6]==0x00) {
        s->rangescale = 0.01; // cm resolution
        printf("1 unit = 1cm, range = %im\n", ranges[s->params[5]]*10);
    }
    else {
        s->rangescale = 1; // XXX.. probably right, it's what
        // we usually ask for, at least.
        printf("Unknown resolution setting!\n");
    }

    return 0;
}

static int sick_config_write(sick_t *s, uint8_t *newparams)
{
    uint8_t payload[SICK_MAX_MSG_LENGTH], request[SICK_MAX_MSG_LENGTH], response[SICK_MAX_MSG_LENGTH];

    sick_config_read(s);

    // don't write if the params are the same. (report success)
    if (!memcmp(s->params, newparams, SICK_PARAMS_LENGTH))
        return 0;

    printf("Sick parameter EEPROM doesn't match.\nOffset  Current  Desired\n");
    for (int i = 0; i < SICK_PARAMS_LENGTH; i++)
      {
	printf("%6d  %6d  %6d\n", i, s->params[i], newparams[i]);
      }

    // switch to config mode
    payload[0] = 0x20;
    payload[1] = 0x00;
    memcpy(&payload[2], "SICK_LMS",8);
    make_telegram(payload, 10, request);

    int res = sick_transaction(s, request, payload[0] | 0x80, response, SICK_CHANGEMODE_TIMEOUT_MS);
    if (res < 0)
        return res;

    if (response[5] != 0x00)
        return -3;

    // reconfigure
    printf("Performing write to SICK EEPROM\n");

    payload[0] = 0x77;
    memcpy(&payload[1], newparams, SICK_PARAMS_LENGTH);
    make_telegram(payload, SICK_PARAMS_LENGTH + 1, request);
    res = sick_transaction(s, request, payload[0] | 0x80, response, SICK_CHANGEMODE_TIMEOUT_MS);
    if (res < 0)
        return res;

    if (response[5] != 1)
        return -4;

    // update was good! remember these settings
    memcpy(s->params, newparams, SICK_PARAMS_LENGTH);

    // switch back to monitoring mode
    payload[0] = 0x20;
    payload[1] = 0x25;

    make_telegram(payload, 2, request);
    res = sick_transaction(s, request, payload[0] | 0x80, response, SICK_CHANGEMODE_TIMEOUT_MS);
    if (res < 0)
        return res;

    if (response[5] != 0x00)
        return -5;

    sick_config_read(s);

    return 0;
}

void sick_make_continuous_request(sick_t *s, uint8_t *request, int enable)
{
    uint8_t payload[2];

    payload[0] = 0x20;

    if (!enable)
        payload[1] = 0x25; // stop continuous mode
    else {                 // start continuous mode
        if (s->interlaced) 
            payload[1] = 0x2a;
        else
            payload[1] = 0x24;
    }

    make_telegram(payload, 2, request);
}

int sick_set_continuous(sick_t *s, int enable)
{
    uint8_t request[SICK_MAX_MSG_LENGTH], response[SICK_MAX_MSG_LENGTH];
    sick_make_continuous_request(s, request, enable);

    int res = sick_transaction(s, request, request[4] | 0x80, response, SICK_OP_TIMEOUT_MS);
    if (res < 0)
        return res;

    s->continuousmode = enable;

    return 0;
}

// angle is e.g. 100 or 180
// resolution is 100ths of a degree, must be 25, 50, or 100.
// if you use 25, the data will be interleaved (interlaced).
int sick_set_variant(sick_t *s, int fov_degrees, int res_cdegrees, int interlaced)
{
    printf("Setting variant: fov = %d, res = %.2f, interlaced = %s\n", fov_degrees, res_cdegrees/100.0, interlaced ? "true" : "false");

    // validate mode
    if (fov_degrees!=100 && fov_degrees!=180)    {
        printf("angle fov must be 100 or 180\n");
        return -7;
    }

    if (res_cdegrees!=25 && res_cdegrees!=50 && res_cdegrees!=100) {
        printf("res_cdegrees must be 25, 50, or 100\n");
        return -8;
    }
    
    if (res_cdegrees == 25) {
        if ((fov_degrees==180 && !interlaced) ||
            (fov_degrees==100 && interlaced)) {
            printf("Can only support non-interlaced 0.25 deg res with 100 FOV.\n");
            return -99;
        }
    }
    if (fov_degrees == 100 && (interlaced || res_cdegrees!=25)) {
        printf("Can only support 100 deg FOV and 0.25 deg resolution mode (non-interlaced)\n");
        return -100;
    }

    // XXX TRICKY HACK XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX! 
    // The Sick firmware has a "bug": if you want 0.25 deg resolution and 180deg FOV, you
    // specify a 100deg FOV or it will reject your variant. The documentation clearly states
    // that the requested FOV is ignored in interlaced mode, and that 180 deg is always used.
    // So this hack is "documented" to work, but it's still the case that the Sick shouldn't
    // reject the variant... Thus, below, we lie about the variant in this case.
    
    int fov_degrees_hack = fov_degrees;

    if (fov_degrees==180 && res_cdegrees==25 && interlaced)
        fov_degrees_hack = 100;

    uint8_t payload[5], request[SICK_MAX_MSG_LENGTH], response[SICK_MAX_MSG_LENGTH];
    payload[0]=0x3b;
    payload[1]=fov_degrees_hack&0xff;
    payload[2]=fov_degrees_hack/256;
    payload[3]=res_cdegrees&0xff;
    payload[4]=res_cdegrees/256;
    make_telegram(payload, 5, request);

    int res = sick_transaction(s, request, payload[0] | 0x80, response, SICK_OP_TIMEOUT_MS);
    if (res < 0)
        return res;

    if (response[5] != 0x01)
        return -6;

    s->fov_degrees = fov_degrees;
    s->res_cdegrees = res_cdegrees;
    s->interlaced = interlaced;

    return 0;
}


int lastcnt=0;
struct timespec ts_scan;

void sick_handle_scan(sick_t *s, uint8_t *packet)
{
    int statusword;
    int ispartialscan;
    int partialscannumber;
    int units;
    int numsamples;

    int i, d;
    int mask_range, mask_intensity;

    statusword = packet[5] + (packet[6] << 8);
    numsamples = statusword & 0x01ff;
    partialscannumber = (statusword >> 11 ) & 0x0003;
    ispartialscan = (statusword>>13) & 0x01;
    units = statusword >> 14;

    float degstep, deg0;

    if (s->max_range_meters == 8 || s->max_range_meters == 80){
        mask_range = 0x1fff;
        mask_intensity = 0xe000;
    }
    else if (s->max_range_meters == 16){
        mask_range = 0x3fff;
        mask_intensity = 0xc000;
    }
    else if (s->max_range_meters == 32){
        mask_range = 0x7fff;
        mask_intensity = 0x8000;
    }
    else { // default to 8m/80m settings
        mask_range = 0x1fff;
        mask_intensity = 0xe000;
    }

    if (ispartialscan) {
        // this is an interleaved scan.
        degstep = 1;
        deg0 = -90 + 0.25*partialscannumber;
    } else {
        // not interleaved

        if (numsamples == 181) {
            deg0 = -90;
            degstep = 1;
        } else if (numsamples == 361) {
            deg0 = -90;
            degstep = 0.5;
        } else if (numsamples == 401) {
            deg0 = -50;
            degstep = 0.25;
        } else {
            printf("couldn't figure out deg0, degstep! numsamples=%d", numsamples);
            return;
        }
    }
    
/*
    // sanity check the # of samples
    int correctnumsamples=s->fov_degrees*100/s->res_cdegrees+1;
    if (numsamples!=correctnumsamples) {
        printf("numsamples=%i, not %i\n",numsamples,correctnumsamples);
        return;
    }
*/

    if (s->callbackdatasize < numsamples) {
        if (s->callbackranges)
            free(s->callbackranges);
        if (s->callbackintensities)
            free(s->callbackintensities);
        s->callbackranges = (float*) calloc(numsamples, sizeof(float));
        s->callbackintensities = (float*) calloc(numsamples, sizeof(float));
        s->callbackdatasize = numsamples;
    }

    for (i=0; i<numsamples; i++) {
        d  =  packet[7+i*2] + (packet[8+i*2]<<8);
        s->callbackranges[i] = (d & mask_range) * s->rangescale;
        s->callbackintensities[i] = (d & mask_intensity)/8.0;
        //s->callbackranges[i] = (d & 0x1fff) * s->rangescale;
        //s->callbackintensities[i] = (d & 0xe000)/8.0;
    }

    // real-time indices
    int runningcnt = packet[7 + numsamples*2];

    int64_t scan_utime = bot_timestamp_sync(s->sync, runningcnt, bot_timestamp_now());

    // call the callback
    if (s->scan_callback != NULL) {
        s->scan_callback(s, s->scan_callback_user, 
                         scan_utime,  
                         deg0*PI/180, degstep*PI/180, numsamples, s->callbackranges, s->callbackintensities);
    }
    if (runningcnt!=(lastcnt+1)%256)
      printf("SICK: WARNING packet loss (okay on start up): curr id:%d prev id:%d\n",runningcnt, 
lastcnt);
    lastcnt = runningcnt;
}

// set sick to our defaults
int sick_config_write_our_settings(sick_t *s)
{
    unsigned char buf[SICK_PARAMS_LENGTH];

    buf[0] =0x00; // A0: maximum diameter of objects to be ignored, e.g. 7 -> 70mm
    buf[1] =0x00; // A1:
    buf[2] =0x46; // B0: peak threshold (LSB)
    buf[3] =0x00; // B1: (MSB) SENSITIVITY: (0=std, 1=med, 2=low, 3=high)
    buf[4] =0x03; // C: send real-time indices (0x02) + Availability level 3 (0x01)

    if (s->max_range_meters == 8) {
	buf[5] =0x01; // D: range/scaling. (0x06=32meters, 0x01=80meters)
        buf[6] =0x01; // E: resolution (0x01=mm, 0x00=cm)
    }
    if (s->max_range_meters == 16) {
        buf[5] =0x03; // D: range/scaling. (0x06=32meters, 0x01=80meters)
        buf[6] =0x01; // E: resolution (0x01=mm, 0x00=cm)
    }
    if (s->max_range_meters == 32) {
        buf[5] =0x06; // D: range/scaling. (0x06=32meters, 0x01=80meters)
        buf[6] =0x01; // E: resolution (0x01=mm, 0x00=cm)
    }
    if (s->max_range_meters == 80) {
        buf[5] =0x01; // D: range/scaling. (0x06=32meters, 0x01=80meters)
        buf[6] =0x00; // E: resolution (0x01=mm, 0x00=cm)
    }

    buf[7] =0x00; // F:
    buf[8] =0x00; // G:
    buf[9] =0x02; // H: multiple evaluation (????) (1-125, 2 default)
    buf[10]=0x02; // I: restart behavior
    buf[11]=0x02; // J: restart time (0x01 current)
    buf[12]=0x00; // K: 2nd multiple evaluation
    buf[13]=0x00; // L: contour A
    buf[14]=0x0a; // M
    buf[15]=0x0a; // N
    buf[16]=0x50; // O
    buf[17]=0x64; // P
    buf[18]=0x00; // Q
    buf[19]=0x0a; // R: contour B
    buf[20]=0x0a; // S
    buf[21]=0x50; // T
    buf[22]=0x64; // U
    buf[23]=0x00; // V
    buf[24]=0x0a; // W
    buf[25]=0x0a; // X
    buf[26]=0x50; // Y
    buf[27]=0x64; // Z
    buf[28]=0x00; // A1 (pixel oriented?)
    buf[29]=0x00; // A2
    buf[30]=0x00; // A3
    buf[31]=0x00;
    buf[32]=0x02; // A4 (number of scans before LMS 2xx switches the
                  // outputs when dazzling occurs (avail level 1 only)
    buf[33]=0x00; // carmen specifies 236 here; that's not even a legal value?

    return sick_config_write(s, buf);
}



