#ifndef BOT_GL_TEXTURE_H
#define BOT_GL_TEXTURE_H

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#endif

/**
 * SECTION:texture
 * @title: Textures
 * @short_description: Rendering images/textures
 * @include: bot2-vis/bot2-vis.h
 *
 * TODO
 *
 * Linking: -lbot2-vis
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _BotGlTexture BotGlTexture;

BotGlTexture * bot_gl_texture_new (int width, int height, int max_data_size);

void bot_gl_texture_free (BotGlTexture * t);

int bot_gl_texture_upload (BotGlTexture * t, GLenum format,
        GLenum type, int stride, void * data);

/**
 * bot_gl_texture_draw:
 *
 * renders the texture in a unit square from (0, 0) to (1, 1).  Texture
 * coordinate mapping:
 *
 * texture          opengl
 * 0, 0          -> 0, 0
 * width, 0      -> 1, 0
 * 0, height     -> 0, 1
 * width, height -> 1, 1
 *
 * all opengl Z coordinates are 0.
 */
void
bot_gl_texture_draw (BotGlTexture * t);

void
bot_gl_texture_draw_coords (BotGlTexture * t, 
        double x_top_left,  double y_top_left,  double z_top_left,
        double x_bot_left,  double y_bot_left,  double z_bot_left,
        double x_bot_right, double y_bot_right, double z_bot_right,
        double x_top_right, double y_top_right, double z_top_right);

/**
 * bot_gl_texture_set_interp:
 * @nearest_or_linear: typically GL_LINEAR or GL_NEAREST.  default is GL_LINEAR
 *
 * sets the interpolation mode when the texture is not drawn at a 1:1 scale.
 */
void bot_gl_texture_set_interp (BotGlTexture * t, GLint nearest_or_linear);

void bot_gl_texture_set_internal_format (BotGlTexture *t, GLenum fmt);

int bot_gl_texture_get_width (BotGlTexture *t);

int bot_gl_texture_get_height (BotGlTexture *t);

GLuint bot_gl_texture_get_texname (BotGlTexture *t);

GLenum bot_gl_texture_get_target (BotGlTexture *t);

#ifdef __cplusplus
}
#endif

#endif
