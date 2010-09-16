#ifndef __bot_fasttrig_h__
#define __bot_fasttrig_h__

/**
 * SECTION:fasttrig
 * @title: Fast Trigonometry
 * @short_description: Very fast, but approximate trigonometry
 * @include: bot2-core/bot2-core.h
 *
 * Linking: -lbot2-core
 */

#ifdef __cplusplus
extern "C" {
#endif

void bot_fasttrig_init(void);
void bot_fasttrig_sincos(double theta, double *s, double *c);
double bot_fasttrig_atan2(double y, double x);

#ifdef __cplusplus
}
#endif

#endif
