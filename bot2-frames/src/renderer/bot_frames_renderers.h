/*
 * coord_frames_renderer.h
 *
 *  Created on: Jan 22, 2011
 *      Author: abachrac
 */
#include <bot_vis/bot_vis.h>
#include <bot_frames/bot_frames.h>

#ifndef COORD_FRAMES_RENDERER_H_
#define COORD_FRAMES_RENDERER_H_

#ifdef __cplusplus
extern "C" {
#endif

void bot_frames_add_renderer_to_viewer(BotViewer *viewer, int render_priority, BotFrames * frames, const char * renderer_name);

void bot_frames_add_articulated_body_renderer_to_viewer(BotViewer *viewer, int render_priority, BotParam * param,
    BotFrames * frames, const char * model_path, const char * param_articulated_name);

#ifdef __cplusplus
}
#endif

#endif /* COORD_FRAMES_RENDERER_H_ */
