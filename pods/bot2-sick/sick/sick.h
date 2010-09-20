#ifndef _SICK_H
#define _SICK_H

#include <stdint.h>
#include <glib.h>

#include <bot2-core/bot2-core.h>

#define SICK_PARAMS_LENGTH 34

typedef struct _sick_t sick_t;

typedef void (*sick_scan_callback_t)(sick_t *sick, void *user, int64_t ts, 
				     float rad0, float radstep,
				     int nranges, float *ranges, float *intensities);

enum SICK_MODE { SICK_MODE_180_ONE, SICK_MODE_180_HALF, SICK_MODE_100_QUARTER,
		 SICK_MODE_180_HALF_INTERLACED, SICK_MODE_180_QUARTER_INTERLACED };

struct sick_serial_ops
{
  void* (*sick_serial_open)(const char *port);
  int   (*sick_serial_getfd)(void *ptr);
  int   (*sick_serial_setbaud)(void *ptr, int baud);
  int   (*sick_serial_close)(void *ptr);
};

struct _sick_t
{
    int sick_serialfd;
    uint8_t *params;
    int paramslength;
    
    int fov_degrees;          // degrees
    int res_cdegrees;         // hundredths of a degree
    int interlaced;           // boolean
    
    int max_range_meters;     // maximum measurement range 
                              // (8m, 16m, 32m, or 80m default)

    int continuousmode; // are we in continuous mode?
    
    float rangescale;     // how many meters per returned range unit?
    
    //pthread_mutex_t writelock; // used for finding responses to requests
    //pthread_cond_t  writecond;
    GMutex *writelock; // used for finding responses to requests
    GCond *writecond;

    uint8_t writereqid;
    uint8_t *writedata;
    uint8_t writevalid;
    
    sick_scan_callback_t scan_callback;
    void *scan_callback_user;
    
    float *callbackranges;
    float *callbackintensities;
    int    callbackdatasize;
    
    bot_timestamp_sync_state_t *sync;
    
    struct sick_serial_ops ops;
    void  *sick_serial_context;
};


/** Create a new sick object. **/
sick_t *sick_create(struct sick_serial_ops *ops);

void sick_destroy(sick_t *s);

/** Connect the sick object to the requested port. The baud rate is
    automatically negotiated. Baudhint can be initialized with a guess
    of the current baud rate; this can decrease initialization
    time. If you don't know, you can pass 0. Returns 0 on success. 

    Possible modes (I = interlaced: you get all the data, but not at once.)

    RES:  1.0     0.5     0.5I     0.25     0.25I
    -----------------------------------------------------------------
FOV 180    X       X       X                  X
    100                             X 

    Note that most 100 deg FOV modes are NOT supported because they
    are strict subsets of the 180 deg modes. I suggest you use the
    interlaced modes; you get high update rate and better timing
    resolution.

    Note that resolution should be 25, 50, 100 (centi-degrees).
**/
int sick_connect(sick_t *s, char *port, int baudhint, int fovdegrees, int resolution, int interlaced, int max_range_meters);

void sick_set_scan_callback(sick_t *s, sick_scan_callback_t callback, void *user);

/** Put the sick into continuous mode, where it constantly sends new
scan data.  Returns 0 on success. **/
int sick_set_continuous(sick_t *s, int enable);

/** Request a baud rate change.  Returns 0 on success. **/
int sick_set_baud(sick_t *s, int baud);

/** Get the Sick scanner type, putting the result into buffer 'buf'.
Returns 0 on success. **/
int sick_get_type(sick_t *s, char *buf, int bufmax);

/** Get the sick scanner status packet, putting the result into buffer
'buf'.  Returns 0 on success. **/
int sick_get_status(sick_t *s, uint8_t *buf, int bufmax);

/** Change the scanning mode.  angle: in degrees (180, usually)
    resolution: hundredths of a degree. [25, 50, 100].  Returns 0 on
    success. **/
int sick_set_variant(sick_t *s, int angle, int resolution, int interlaced);

/** Request a new scan.  Returns 0 on success. **/
int sick_request_scan(sick_t *s);

#endif
