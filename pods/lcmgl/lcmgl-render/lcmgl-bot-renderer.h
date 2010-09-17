#ifndef __lcmgl_bot_renderer_h__
#define __lcmgl_bot_renderer_h__

#include <lcm/lcm.h>

#include <bot2-vis/bot2-vis.h>

#ifdef __cplusplus
extern "C" {
#endif

void lcmgl_add_renderer_to_bot_viewer(BotViewer* viewer, lcm_t* lcm, int priority);

#ifdef __cplusplus
}
#endif

#endif
