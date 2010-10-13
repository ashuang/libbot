#ifndef __bgl_h__
#define __bgl_h__

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
//#include <GL/freeglut.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
//#include <GL/freeglut.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup BotVisBatchGL Batched OpenGL commands
 * @brief Batching OpenGL commands for delayed rendering
 * @ingroup BotVis
 * @include: bot2-vis/bot2-vis.h
 *
 * Batched GL drawing allows a program to issue some basic OpenGL drawing
 * commands while an OpenGL context is not active.  Commands will be queued up,
 * stored, and then executed when requested via bot_bgl_render().
 *
 * Linking: `pkg-config --libs bot2-vis`
 *
 * @{
 */

typedef struct _bot_bgl bot_bgl_t;

bot_bgl_t *bot_bgl_new (void);
void bot_bgl_destroy (bot_bgl_t *bgl);

void bot_bgl_begin (bot_bgl_t *bgl, GLenum mode);
void bot_bgl_end (bot_bgl_t *bgl);

void bot_bgl_vertex2d (bot_bgl_t *bgl, double v0, double v1);
void bot_bgl_vertex2dv(bot_bgl_t *bgl, const double *v);
void bot_bgl_vertex2f (bot_bgl_t *bgl, float v0, float v1);
void bot_bgl_vertex2fv(bot_bgl_t *bgl, const float *v);

void bot_bgl_vertex3d (bot_bgl_t *bgl, double v0, double v1, double v2);
void bot_bgl_vertex3dv(bot_bgl_t *bgl, const double *v);
void bot_bgl_vertex3f (bot_bgl_t *bgl, float v0, float v1, float v2);
void bot_bgl_vertex3fv(bot_bgl_t *bgl, const float *v);

void bot_bgl_color3f (bot_bgl_t *bgl, float v0, float v1, float v);
void bot_bgl_color4f (bot_bgl_t *bgl, float v0, float v1, float v2, float v3);
void bot_bgl_point_size (bot_bgl_t *bgl, float v);
void bot_bgl_line_width (bot_bgl_t *bgl, float line_width);

void bot_bgl_enable (bot_bgl_t *bgl, GLenum v);
void bot_bgl_disable (bot_bgl_t *bgl, GLenum v);

void bot_bgl_blend_func (bot_bgl_t *bgl, GLenum sfactor, GLenum dfactor);

void bot_bgl_translated (bot_bgl_t *bgl, double v0, double v1, double v2);
void bot_bgl_translatef (bot_bgl_t *bgl, float v0, float v1, float v2);
void bot_bgl_rotated (bot_bgl_t *bgl, double angle, double x, double y, double z);
void bot_bgl_rotatef (bot_bgl_t *bgl, float angle, float x, float y, float z);
void bot_bgl_push_matrix (bot_bgl_t * bgl);
void bot_bgl_pop_matrix (bot_bgl_t * bgl);
void bot_bgl_load_identity (bot_bgl_t *bgl);

void bot_bgl_mult_matrixd (bot_bgl_t *bgl, const double *matrix);
void bot_bgl_mult_matrixf (bot_bgl_t *bgl, const float *matrix);

void bot_bgl_render (bot_bgl_t *bgl);
void bot_bgl_switch_buffer (bot_bgl_t *bgl);

//void bot_bgl_box_3dv_3fv(bot_bgl_t *bgl, double xyz[3], float dim[3]);
//void bot_bgl_circle(bot_bgl_t *bgl, double xyz[3], double radius);
//void bot_bgl_disk(bot_bgl_t *bgl, double xyz[3], double r_in, double r_out);
//void bot_bgl_text(bot_bgl_t *bgl, const double xyz[3], const char *text);
//void bot_bgl_text_ex(bot_bgl_t *bgl, const double xyz[3], const char *text, uint32_t font, uint32_t flags);
//
//void bot_bgl_rect(bot_bgl_t *bgl, double xyz[3], double size[2], double theta_rad, int filled);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
