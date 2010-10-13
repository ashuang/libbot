#ifndef __bot_gl_console_h__
#define __bot_gl_console_h__

/**
 * @defgroup console Scrolling text OpenGL overlay
 * @brief Vertically scrolling text
 * @ingroup BotVisGl
 * @include: bot2-vis/bot2-vis.h
 *
 * TODO
 *
 * Linking: `pkg-config --libs bot2-vis`
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _BotGlConsole BotGlConsole;

BotGlConsole *bot_gl_console_new (void);
void bot_gl_console_destroy (BotGlConsole *console);

void bot_gl_console_set_glut_font (BotGlConsole *console, void *font);

//void bot_gl_console_set_pos (BotGlConsole *console, double x, double y);

void bot_gl_console_set_decay_lambda (BotGlConsole *console, double lambda);

void bot_gl_console_color3f (BotGlConsole *console, float r, float g, float b);

void bot_gl_console_printf (BotGlConsole *console, const char *format, ...);

void bot_gl_console_render (BotGlConsole *console);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
