#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <signal.h>
#include <sys/time.h>
#include <math.h>
#include <getopt.h>

#include <lcm/lcm.h>

#include <bot_core/bot_core.h>

#include "sick.h"
#include "moxa.h"

typedef struct state state_t;


struct state
{
    bot_timestamp_sync_state_t *sync;
    lcm_t *lcm;
    sick_t *sick;

    int64_t scan_last_ts;          // time of the last scan

    GMutex *report_mutex;
    //pthread_mutex_t report_mutex;

    int64_t report_last_utime;
    int     report_scancount_in;  // scans received during report interval
    int     report_scancount_out; // scans published during report interval
    int64_t report_max_lag;       // maximum delay between any two scans during report interval
    int64_t report_us;            // target report rate

    int64_t publish_next_ts;      // time of our last sent message
    int     publish_us;           // target publish rate

    int     fail_count;

    // options previously part of gopt
    char *lcm_channel;
    int eofailure;
};

static void on_report_timer(state_t *state)
{
    //pthread_mutex_lock(&state->report_mutex);
    g_mutex_lock(state->report_mutex);

    int64_t now = bot_timestamp_now();

    double dt = (now - state->report_last_utime)/1000000.0;
    
    printf("IN: %4.1f Hz   OUT: %4.1f Hz   Max Period: %7.3f ms\n",
           state->report_scancount_in / dt,
           state->report_scancount_out / dt,
           state->report_max_lag / 1000.0);

    if (state->report_scancount_in == 0) {
        state->fail_count++;
        //if (state->fail_count == 3 && getopt_get_bool(state->gopt, "exit-on-failure"))
        if (state->fail_count == 3 && state->eofailure == 1)
            exit(-1);
    } else {
        state->fail_count = 0;
    }

    
    state->report_max_lag = 0;
    state->report_last_utime = now;
    state->report_scancount_in = 0;
    state->report_scancount_out = 0;

    //pthread_mutex_unlock(&state->report_mutex);
    g_mutex_unlock(state->report_mutex);

}

static void my_scan_callback(sick_t *sick, void *user, int64_t ts, 
                      float rad0, float radstep, int nranges, float *ranges, float *intensities)
{
    state_t *state = (state_t*) user;
    bot_core_planar_lidar_t laser;

    laser.utime = ts;
    laser.rad0 = rad0;
    laser.radstep = radstep;

    laser.nranges = nranges;
    laser.ranges = ranges;

    /*
    // XXXXX
    // LSF: We seem to have ported the dgc "trunk" sick driver instead of the dgc "exp" driver, so intensity support is broken.
    // Don't know if there is any reason not to port the exp version in place of this.  
    // For now just not populating intensities in LCM message to halve bandwidth.
    // XXXXX
    if (intensities == NULL) {
      laser.nintensities = 0;
      laser.intensities = NULL;
    } else {
        laser.nintensities = nranges;
        laser.intensities = intensities;
    }
    */
    laser.nintensities = 0;
    laser.intensities = NULL;
    // XXXXX

    bot_core_planar_lidar_t_publish(state->lcm, state->lcm_channel, &laser);

    int64_t now = bot_timestamp_now();
    //pthread_mutex_lock(&state->report_mutex);
    g_mutex_lock(state->report_mutex);

    int64_t lag = now - state->scan_last_ts;
    if (lag > state->report_max_lag)
        state->report_max_lag = lag;
    state->scan_last_ts = now;

    state->report_scancount_in++;
    state->report_scancount_out++;

    //pthread_mutex_unlock(&state->report_mutex);
    g_mutex_unlock(state->report_mutex);
}

static void usage()
{
    fprintf (stderr, "usage: sick [options]\n"
            "\n"
            "  -h, --help             shows this help text and exits\n"
            "  -c, --channel <s>      LCM channel name\n"
            "  -i, --interlaced       interlaced (required for most high-res modes)\n"
            "  -e, --exit-on-failure  exit -1 if sick connection fails\n"
            "  -d, --devicename <s>   device to connect to\n"
            "  -b, --baud <i>         baud rate\n"
            "  -r, --resolution <i>   angular resolution (hundreths of a degree)\n"
            "  -f, --fov <i>          azimuthal field of view (degrees)\n"
            "  -z, --hertz <i>        target update rate (hertz)\n"
            "  -R, --measrange <i>    max measured range (meters) (8, 16, 32, 80 default)\n"
             );
}


int main(int argc, char *argv[])
{
    int res;
    state_t *state = (state_t*) calloc(1, sizeof(state_t));
    
    setlinebuf (stdout);
    

    char *optstring = "hc:ied:b:r:f:z:p:R:";
    int c;
    struct option long_opts[] = {
        {"help", no_argument, 0, 'h'},
        {"channel", optional_argument, 0, 'c'},
        {"interlaced", no_argument, 0, 'i'},
        {"exit-on-failure", no_argument, 0, 'e'},
        {"devicename", optional_argument, 0, 'd'},
        {"baud", optional_argument, 0, 'b'},
        {"resolution", optional_argument, 0, 'r'},
        {"fov", optional_argument, 0, 'f'},
        {"hertz", optional_argument, 0, 'z'},
        {"provider", optional_argument, 0, 'p'},
        {"measrange", optional_argument, 0, 'R'},
        {0, 0, 0, 0}
    };

    // set input argument defaults
    char *lcm_channel = "PLANAR_LIDAR";
    char *devicename = "/dev/sick";
    char *lcm_provider = NULL;
    int resolution = 25; 
    int interlaced = 1;
    int baud = 500000;
    int fov = 180;
    int hertz = 15;
    int max_range_meters = 80;
    state->eofailure = 1;
    state->lcm_channel = strdup (lcm_channel);
        


    // VERIFY THE DEFAULT FOR INTERLACED FLAG. MAKE 0 OR CHANGE TO NON-INTERLACED
    
    while ((c = getopt_long (argc, argv, optstring, long_opts, 0)) >= 0)
        {
        switch (c) 
            {
            case 'c':
                state->lcm_channel = strdup (optarg);
                break;
            case 'i':
                interlaced = 1;
                break;
            case 'e':
                state->eofailure = 1;
                break;
            case 'd':
                devicename = strdup (optarg);
                break;
            case 'b':
                baud = atoi (optarg);
                break;
            case 'r':
                resolution = atoi (optarg);
                break;
            case 'f':
                fov = atoi (optarg);
                break;
            case 'z':
                hertz = atoi (optarg);
                break;
            case 'p':
                lcm_provider = strdup (optarg);
                break;
            case 'R':
                max_range_meters = atoi (optarg);
                break;
            case 'h':
            default:
                usage();
                return 1;
            }
    }



    state->lcm = lcm_create(lcm_provider);
    //pthread_mutex_init(&state->report_mutex, NULL); // moved to after sick_connect, which creates the thread
    // create the mutex
    state->report_mutex = g_mutex_new();


    if (!strncmp(devicename, "moxa:", 5))
        state->sick=sick_create(&moxa_serial_ops); // MOXA serial-to-ethernet server
    else
        state->sick=sick_create(NULL); // default serial

    if ((res=sick_connect(state->sick, devicename, baud, fov, resolution, interlaced, max_range_meters)))                      
    {
        printf("Couldn't connect to scanner. Exiting. (%i)\n",res);                
        perror("Error");
        return -1;
    }


    if (sick_set_baud(state->sick, baud))
    {
        printf("Couldn't change baud. Exiting.\n");
        return -1;
    }

    int64_t now = bot_timestamp_now();
    state->scan_last_ts = now;
    state->report_last_utime = 0;
    state->report_scancount_in = 0;
    state->report_scancount_out = 0;
    state->report_max_lag = 0;
    state->report_us = 5000000;
    state->publish_next_ts = 0;
    state->publish_us = 1000000/hertz;

    sick_set_scan_callback(state->sick, my_scan_callback, state);

    // start continuous operation mode
    sick_set_continuous(state->sick, 1);

    while(1) {
        sleep(2);
        on_report_timer(state);
    }
}
