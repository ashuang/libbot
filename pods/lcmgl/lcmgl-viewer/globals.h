#ifndef __lcmgl_viewer_globals_h__
#define __lcmgl_viewer_globals_h__

// file:  globals.h
// desc:  prototypes for accessing global/singleton objects -- objects that
//        will typically be created once throughout the lifespan of a program.

#include <lcm/lcm.h>

#ifdef __cplusplus
extern "C" {
#endif

lcm_t * globals_get_lcm (void);

#ifdef __cplusplus
}
#endif

#endif
