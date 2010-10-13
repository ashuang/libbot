#ifndef __bot_rwx_h__
#define __bot_rwx_h__

/**
 * @defgroup BotVisRwx Renderware (.rwx) mesh model rendering
 * @brief Parsing and rendering .rwx models
 * @ingroup BotVisGl
 * @include: bot_vis/bot_vis.h
 *
 * Linking: `pkg-config --libs bot2-vis`
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <glib.h>

typedef struct _bot_rwx_vertex
{
    double pos[3];
    int id;
} bot_rwx_vertex_t;

typedef struct _bot_rwx_triangle
{
    int vertices[3];
} bot_rwx_triangle_t;

typedef struct _bot_rwx_clump
{
    double color[3];
//    double surface[3];
    double diffuse;
    double specular;
    double opacity;
    double ambient;
    char *name;
    bot_rwx_vertex_t *vertices;
    bot_rwx_triangle_t *triangles;
    int nvertices;
    int ntriangles;
} bot_rwx_clump_t;

typedef struct _bot_rwx_model
{
    GList *clumps;
    int nclumps;
} bot_rwx_model_t;


bot_rwx_model_t * bot_rwx_model_create( const char *fname );

void bot_rwx_model_destroy( bot_rwx_model_t *model );

void bot_rwx_model_apply_transform( bot_rwx_model_t *model, double m[16]);

void bot_rwx_model_gl_draw( bot_rwx_model_t *model );

void bot_rwx_model_get_extrema (bot_rwx_model_t * model,
        double minv[3], double maxv[3]);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
