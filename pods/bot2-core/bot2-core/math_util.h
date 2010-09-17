#ifndef __bot_mathutil_h__
#define __bot_mathutil_h__

#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

/**
 * SECTION:math_util
 * @title: Math Utilities
 * @short_description: Miscellaneous math utility functions
 * @include: bot2-core/bot2-core.h
 *
 * Linking: -lbot2-core
 */

#ifdef __cplusplus
extern "C" {
#endif


/** valid only for v > 0 **/
static inline double bot_mod2pi_positive(double vin)
{
    double q = vin / (2*M_PI) + 0.5;
    int qi = (int) q;

    return vin - qi*2*M_PI;
}

/** Map v to [-PI, PI] **/
static inline double bot_mod2pi(double vin)
{
    if (vin < 0)
        return -bot_mod2pi_positive(-vin);
    else
        return bot_mod2pi_positive(vin);
}

/** Return vin such that it is within PI degrees of ref **/
static inline double bot_mod2pi_ref(double ref, double vin)
{
    return ref + bot_mod2pi(vin - ref);
}

static inline int bot_theta_to_int(double theta, int max)
{
    theta = bot_mod2pi_ref(M_PI, theta);
    int v = (int) (theta / ( 2 * M_PI ) * max);

    if (v==max)
        v = 0;

    assert (v >= 0 && v < max);

    return v;
}

#define bot_to_radians(deg) ((deg)*M_PI/180)

#define bot_to_degrees(rad) ((rad)*180/M_PI)

#define bot_sq(a) ((a)*(a))

#ifdef __cplusplus
}
#endif

#endif
