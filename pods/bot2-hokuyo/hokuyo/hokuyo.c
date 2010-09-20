/**
 * LCM module for connecting to a Hokuyo laser range finder (URG and UTM are
 * supported) and transmitting range data via LCM.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <sys/time.h>
#include <time.h>

#include <glib.h>
#include <lcm/lcm.h>

#define MAX_ACM_DEVS 20

#include <bot_core/bot_core.h>

#define TO_DEGREES(rad) ((rad)*180/M_PI)

#include "liburg/urg_ctrl.h"

static void
usage(const char *progname)
{
    fprintf (stderr, "usage: %s [options]\n"
            "\n"
            "  -h, --help             shows this help text and exits\n"
            "  -c, --channel CHAN     LCM channel name\n"
            "  -d, --device DEV       Device file to connect to\n"
            "  -i, --id ID            Search for Hokuyo with serial id ID\n"
            "  -l, --lcmurl URL       LCM URL\n"
            "  -s, --skipscans NUM    Publish every NUMth scan [defaults to 1]\n"
            "                           (reduce scan frequency)\n"
            "  -b, --skipbeams NUM    Read every NUMth beams  [defaults to 1]\n"
            "                           (reduce angular resolution)\n"
            "  -I, --intensity        Read Intensity data from the laser scanner\n"
            "                            MAY NOT WORK WITH URGs\n"
            "  -n, --no-timesync      Use the computer's clock to timestamp messages\n"
            "                             (as opposed to using the hokuyo timestamps)\n"
            , g_path_get_basename(progname));
}

static char **
_get_acm_devnames(void)
{
    char **result = (char**)calloc(1, (MAX_ACM_DEVS+1)*sizeof(char*));
    int n = 0;
    for (int i=0;i<MAX_ACM_DEVS;i++) {
        char devname[256];
        sprintf(devname,"/dev/ttyACM%d",i);
        if(g_file_test(devname, G_FILE_TEST_EXISTS)) {
            result[n] = g_strdup(devname);
            n++;
        }
    }
    return result;
}

static int
_connect_by_device(urg_t *urg, urg_parameter_t *params, const char *device)
{
    if(urg_connect(urg, device, 115200) < 0) {
        return 0;
    }
    urg_parameters(urg, params);
    return 1;
}

static int
_connect_any_device(urg_t *urg, urg_parameter_t *params)
{
    char **devnames = _get_acm_devnames();
    if(!devnames[0]) {
        printf("No Hokuyo detected\n");
    }
    for(int i=0; devnames[i]; i++) {
        printf("Trying %s...\n", devnames[i]);
        if(_connect_by_device(urg, params, devnames[i])) {
            g_strfreev(devnames);
            return 1;
        }
    }
    g_strfreev(devnames);
    return 0;
}

static gboolean
_read_serialno(urg_t *urg, int *serialno)
{
    // read the serial number of the hokuyo.  This is buried within
    // a bunch of other crap.
    int LinesMax = 5;
    char version_buffer[LinesMax][UrgLineWidth];
    char *version_lines[LinesMax];
    for (int i = 0; i < LinesMax; ++i) {
        version_lines[i] = version_buffer[i];
    }
    int status = urg_versionLines(urg, version_lines, LinesMax);
    if (status < 0) {
        fprintf(stderr, "urg_versionLines: %s\n", urg_error(urg));
        return 0;
    }
    const char *prefix = "SERI:";
    int plen = strlen(prefix);

    for(int i = 0; i < LinesMax; ++i) {
        if(!strncmp(version_lines[i], prefix, plen)) {
            char *eptr = NULL;
            int sn = strtol(version_lines[i] + plen, &eptr, 10);
            if(eptr != version_lines[i] + plen) {
                *serialno = sn;
                return 1;
            }
        }
    }
    return 0;
}

static gboolean
_connect_by_id(urg_t *urg, urg_parameter_t *params, const int desired_serialno)
{
    char **devnames = _get_acm_devnames();
    for(int i=0; devnames[i]; i++) {
        printf("Trying %s...\n", devnames[i]);
        const char *devname = devnames[i];

        if(!_connect_by_device(urg, params, devname)) {
            continue;
        }

        int serialno = -1;
        if(!_read_serialno(urg, &serialno)) {
            printf("Couldn't read serial number on %s\n", devname);
            urg_laserOff(urg);
            urg_disconnect(urg);
            continue;
        }

        if(desired_serialno == serialno) {
            printf("Found %d on %s\n", serialno, devname);
            g_strfreev(devnames);
            return 1;
        } else {
            printf("Skipping %s (found serial #: %d, desired: %d)\n", devname, serialno, desired_serialno);
            urg_laserOff(urg);
            urg_disconnect(urg);
        }
    }
    g_strfreev(devnames);
    return 0;
}

static gboolean
_connect(urg_t *urg, urg_parameter_t *params, int serialno,
        const char *device, int *data_max, bot_timestamp_sync_state_t **sync, int skipscans, int skipbeams, int readIntensities)
{
    if(serialno) {
        if(!_connect_by_id(urg, params, serialno)) {
            return 0;
        }
    } else if(device) {
        if(!_connect_by_device(urg, params, device))
            return 0;
    } else {
        if(!_connect_any_device(urg, params)) {
            return 0;
        }
    }

    if(data_max)
        *data_max = urg_dataMax(urg);

    // read and print out version information
    int LinesMax = 5;
    char version_buffer[LinesMax][UrgLineWidth];
    char *version_lines[LinesMax];
    for (int i = 0; i < LinesMax; ++i) {
        version_lines[i] = version_buffer[i];
    }
    int status = urg_versionLines(urg, version_lines, LinesMax);
    if (status < 0) {
        fprintf(stderr, "urg_versionLines: %s\n", urg_error(urg));
        urg_disconnect(urg);
        return 0;
    }
    for(int i = 0; i < LinesMax; ++i) {
        printf("%s\n", version_lines[i]);
    }
    printf("\n");

    if(*sync)
        bot_timestamp_sync_free(*sync);
    *sync = bot_timestamp_sync_init(1000, 4294967296L, 1.001);
    // guessed at wrap-around based on 4 byte field.

    // estimate clock skew
    urg_enableTimestampMode(urg);
    for (int i = 0; i < 10; i++) {
        int64_t hokuyo_mtime = urg_currentTimestamp(urg);
        int64_t now = bot_timestamp_now();
        bot_timestamp_sync(*sync, hokuyo_mtime, now);
    }
    urg_disableTimestampMode(urg);

    // configure Hokuyo to continuous capture mode.
    urg_setCaptureTimes(urg, UrgInfinityTimes);

    // set the scan skip
    //For whatever reason, the hokuyo API is inconsistent between the skip scans and skip beams
    //skip scans means skip this many scans between every one that gets read
    //so 0 would be full rate
    //skip beams means read every SKIPBEAMth reading,
    //so 1 is full angular resolution
    //the -1 makes the two consistent
    urg_setSkipFrames(urg,skipscans-1);

    // set the beam skip
    urg_setSkipLines(urg,skipbeams);

    // start data transmission
    if (readIntensities)
      status = urg_requestData(urg, URG_MD_INTENSITY, URG_FIRST, URG_LAST);
    else
      status = urg_requestData(urg, URG_MD, URG_FIRST, URG_LAST);

    if (status < 0) {
        fprintf(stderr, "urg_requestData(): %s\n", urg_error(urg));
        urg_disconnect(urg);
        return 0;
    }

    return 1;
}

int main(int argc, char *argv[])
{
    setlinebuf(stdout);

    char *optstring = "hc:d:p:i:as:b:In";
    char c;
    struct option long_opts[] = {
        {"help", no_argument, 0, 'h'},
        {"channel", required_argument, 0, 'c'},
        {"device", required_argument, 0, 'd'},
        {"id", required_argument, 0, 'i'},
        {"lcmurl", required_argument, 0, 'l'},
        {"skipscans",required_argument,0,'s'},
        {"skipbeams",required_argument,0,'b'},
        {"intensity", no_argument, 0, 'I'},
        {"no-timesync", no_argument,0,'n'},
        {0, 0, 0, 0}
    };

    int exit_code = 0;
    char *device = NULL;
    char *channel = g_strdup("HOKUYO_LIDAR");
    char *lcm_url = NULL;
    int serialno = 0;
    int skipscans =1;
    int skipbeams =1;
    int readIntensities = 0;
    int no_timesync = 0;

    while ((c = getopt_long (argc, argv, optstring, long_opts, 0)) >= 0)
    {
        switch (c)
        {
            case 'c':
                free(channel);
                channel = g_strdup(optarg);
                break;
            case 'd':
                free(device);
                device = g_strdup(optarg);
                break;
            case 'p':
                free(lcm_url);
                lcm_url = g_strdup(optarg);
                break;
            case 'i':
                {
                    char *eptr = NULL;
                    serialno = strtol(optarg, &eptr, 10);
                    if(*eptr != '\0') {
                        usage(argv[0]);
                        return 1;
                    }
                }
                break;
            case 's':
                {
                    skipscans = atoi(optarg);
                    if (skipscans<1){
                        usage(argv[0]);
                        return 1;
                    }
                }
                break;
            case 'b':
                {
                    skipbeams = atoi(optarg);
                    if (skipbeams<1){
                        usage(argv[0]);
                        return 1;
                    }
                }
                break;
            case 'I':
                readIntensities =1;
                break;
            case 'n':
                no_timesync =1;
                break;
            case 'h':
            default:
                usage(argv[0]);
                return 1;
        }
    }

    int data_max;
    long* data = NULL;
    long* intensity = NULL;
    urg_parameter_t urg_param;

    // setup LCM
    lcm_t *lcm = lcm_create(lcm_url);
    if(!lcm) {
        fprintf(stderr, "Couldn't setup LCM\n");
        return 1;
    }

    bot_timestamp_sync_state_t *sync = NULL;

    urg_t urg;
    int max_initial_tries = 10;
    int connected = 0;
    for(int i=0; i<max_initial_tries && !connected; i++) {
        connected = _connect(&urg, &urg_param, serialno, device, &data_max, &sync, skipscans,skipbeams,readIntensities);
        if(!connected) {
            struct timespec ts = { 0, 500000000 };
            nanosleep(&ts, NULL);
        }
    }
    if(!connected) {
        fprintf(stderr, "Unable to connect to any device\n");
        lcm_destroy(lcm);
        return 1;
    }

    // # of measurements per scan?
    data = (long*)malloc(sizeof(long) * data_max);
    intensity = (long*)malloc(sizeof(long) * data_max);
    if (data == NULL || intensity ==NULL) {
        perror("data buffer");
        exit_code = 1;
        goto done;
    }

    bot_core_planar_lidar_t msg;
    int max_nranges = urg_param.area_max_ - urg_param.area_min_ + 1;
    msg.ranges = (float*) malloc(sizeof(float) * max_nranges);
    msg.nintensities = 0;
    msg.intensities = (float*) malloc(sizeof(float) * max_nranges);
    msg.radstep = 2.0 * M_PI / urg_param.area_total_ * skipbeams;
    msg.rad0 = urg_index2rad(&urg,urg_param.area_min_);
    printf("Area Max: %d  Area Min: %d\n", urg_param.area_max_, urg_param.area_min_);
    printf("Angular resolution: %f deg\n", TO_DEGREES(msg.radstep));
    printf("Starting angle:     %f deg\n", TO_DEGREES(msg.rad0));
    printf("Scan RPM:           %d\n", urg_param.scan_rpm_);
    printf("\n");

    int64_t now = bot_timestamp_now();
    int64_t report_last_utime = now;
    int64_t report_interval_usec = 2000000;
    int64_t next_report_utime = now + report_interval_usec;

    int64_t scancount_since_last_report = 0;
    int failure_count = 0;
    int reconnect_thresh = 10;
    int epic_fail = 0;
    int max_reconn_attempts = 600;

    // loop forever, reading scans
    while(!epic_fail) {
        int nranges=-1;
        if (!readIntensities)
            nranges = urg_receiveData(&urg, data, data_max);
        else
            nranges = urg_receiveDataWithIntensity(&urg, data, data_max,intensity);

        if(nranges > max_nranges) {
             printf("WARNING:  received more range measurements than the maximum advertised!\n");
             printf("          Hokuyo reported max %d, but received %d\n", max_nranges, nranges);
             max_nranges = nranges;
             msg.ranges = (float*) realloc(msg.ranges, sizeof(float) * max_nranges);
             if (readIntensities)
               msg.intensities = (float*) realloc(msg.intensities, sizeof(float) * max_nranges);
        }
        now = bot_timestamp_now();
        int64_t hokuyo_mtime = urg_recentTimestamp(&urg);

        if(nranges < 0) {
            // sometimes, the hokuyo can freak out a little.
            // Count how many times we've failed to get data from the hokuyo
            // If it's too many times, then reset the connection.  That
            // can help sometimes..
            fprintf(stderr, "urg_receiveData(): %s\n", urg_error(&urg));
            failure_count++;
            struct timespec ts = { 0, 300000000 };
            nanosleep(&ts, NULL);

            int reconn_failures = 0;
            while(failure_count > reconnect_thresh) {
                if(connected) {
                    urg_disconnect(&urg);
                    connected = 0;
                    fprintf(stderr, "Comms failure.  Trying to reconnect...\n");
                }

                if(_connect(&urg, &urg_param, serialno, device, NULL, &sync,skipscans,skipbeams, readIntensities)) {
                    failure_count = 0;
                    connected = 1;
                }

                // Throttle reconnect attempts
                struct timespec ts = { 0, 500000000 };
                nanosleep(&ts, NULL);

                reconn_failures++;
                if(reconn_failures > max_reconn_attempts) {
                    fprintf(stderr, "Exceeded maximum reconnection attempts.\n");
                    exit_code = 1;
                    epic_fail = 1;
                    break;
                }
            }
            continue;
        }

        if(failure_count > 0)
            failure_count--;

        if (no_timesync)
          msg.utime = now;
        else
          msg.utime = bot_timestamp_sync(sync, hokuyo_mtime, now);

        msg.nranges = nranges/skipbeams;
        if (readIntensities){
          msg.nintensities = nranges/skipbeams;
          int c=0;
          for(int i=0; i<nranges; i+=skipbeams) {
            msg.ranges[c] = data[i] * 1e-3;
            msg.intensities[c] = intensity[i]; //TODO: this should probably be scaled???
            c++;
          }
        }
        else{
          int c=0;
          for(int i=0; i<nranges; i+=skipbeams)
            msg.ranges[c++] = data[i] * 1e-3;
        }
        bot_core_planar_lidar_t_publish(lcm, channel, &msg);

        scancount_since_last_report++;

        if(now > next_report_utime) {
            double dt = (now - report_last_utime) * 1e-6;

            printf("%4.1f Hz\n", scancount_since_last_report / dt);

            scancount_since_last_report = 0;
            next_report_utime = now + report_interval_usec;
            report_last_utime = now;
        }
    }

done:
    lcm_destroy(lcm);
    if(connected) {
        urg_laserOff(&urg);
        urg_disconnect(&urg);
    }

    if(sync)
        bot_timestamp_sync_free(sync);
    free(data);
    free(intensity);
    free(lcm_url);
    free(channel);
    free(device);

    return exit_code;
}
