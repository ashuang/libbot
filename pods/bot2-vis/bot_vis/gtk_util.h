#ifndef __gtk_util_h__
#define __gtk_util_h__

#include <gtk/gtk.h>
#include "param_widget.h"
#include "gl_drawing_area.h"
#include "gl_image_area.h"
#include "viewer.h"

/**
 * SECTION:gtk_util
 * @title: GTK Utilities
 * @short_description: GTK utility functions
 * @include: bot2-vis/bot2-vis.h
 *
 * TODO
 *
 * Linking: -lbot2-vis
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Adds an event handler to the GTK mainloop that calls gtk_main_quit() when 
 * SIGINT, SIGTERM, or SIGHUP are received
 */
int bot_gtk_quit_on_interrupt(void);

#ifdef __cplusplus
}
#endif

#endif
