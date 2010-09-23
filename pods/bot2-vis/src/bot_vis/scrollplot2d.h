#ifndef __bot_gl_scrollplot2d_h__
#define __bot_gl_scrollplot2d_h__

/**
 * SECTION:scrollplot2d
 * @title: Scrolling Plots
 * @short_description: Plotting windows of time-varying signals
 * @include: bot2-vis/bot2-vis.h
 *
 * TODO
 *
 * Linking: -lbot2-vis
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BOT_GL_SCROLLPLOT2D_HIDDEN,
    BOT_GL_SCROLLPLOT2D_TOP_LEFT,
    BOT_GL_SCROLLPLOT2D_TOP_RIGHT,
    BOT_GL_SCROLLPLOT2D_BOTTOM_LEFT,
    BOT_GL_SCROLLPLOT2D_BOTTOM_RIGHT
} BotGlScrollPlot2dLegendLocation;

typedef struct _BotGlScrollPlot2d BotGlScrollPlot2d;

BotGlScrollPlot2d * bot_gl_scrollplot2d_new (void);

void bot_gl_scrollplot2d_free (BotGlScrollPlot2d *self);

int bot_gl_scrollplot2d_set_title (BotGlScrollPlot2d *self, const char *title);

int bot_gl_scrollplot2d_set_text_color (BotGlScrollPlot2d *self, 
        double r, double g, double b, double a);

void bot_gl_scrollplot2d_set_show_title (BotGlScrollPlot2d *self, int val);

//void bot_gl_scrollplot2d_set_show_ylim (BotGlScrollPlot2d *self, int val);

int bot_gl_scrollplot2d_set_bgcolor (BotGlScrollPlot2d *self, 
        double r, double g, double b, double alpha);

int bot_gl_scrollplot2d_set_border_color (BotGlScrollPlot2d *self, 
        double r, double g, double b, double alpha);

int bot_gl_scrollplot2d_set_show_legend (BotGlScrollPlot2d *self,
        BotGlScrollPlot2dLegendLocation where);

int bot_gl_scrollplot2d_set_xlim (BotGlScrollPlot2d *self, double xmin, 
        double xmax);

int bot_gl_scrollplot2d_set_ylim (BotGlScrollPlot2d *self, double ymin, 
        double ymax);

int bot_gl_scrollplot2d_add_plot (BotGlScrollPlot2d *self, const char *name,
        int max_points);

int bot_gl_scrollplot2d_remove_plot (BotGlScrollPlot2d *self, const char *name);

int bot_gl_scrollplot2d_set_color (BotGlScrollPlot2d *self, const char *name, 
        double r, double g, double b, double alpha);

int bot_gl_scrollplot2d_add_point (BotGlScrollPlot2d *self, const char *name,
        double x, double y);

/**
 * renders the plot in a square from [ 0, 0 ] to [ 1, 1 ]
 */
//void bot_gl_scrollplot2d_gl_render (BotGlScrollPlot2d *self);

void bot_gl_scrollplot2d_gl_render_at_window_pos (BotGlScrollPlot2d *self,
        int topleft_x, int topleft_y, int width, int height);

#ifdef __cplusplus
}
#endif

#endif
