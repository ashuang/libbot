#ifndef __lcmgl_h__
#define __lcmgl_h__

#include <lcm/lcm.h>

/**
 * SECTION:lcmgl
 * @title: LCMGL
 * @short_description: OpenGL rendering via LCM - client routines
 * @include: lcmgl/lcmgl_client.h
 *
 * TODO
 *
 * Linking: -llcmgl-client
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lcmgl lcmgl_t;

/**
 * lcmgl_init:  
 * 
 * Constructor
 */
lcmgl_t *lcmgl_init(lcm_t *lcm, const char *name);

/**
 * lcmgl_destroy:  
 * 
 * Destructor
 */
void lcmgl_destroy(lcmgl_t *lcmgl);

/**
 * lcmgl_switch_buffer:
 *
 * Transmits all queued operations.
 */
void lcmgl_switch_buffer(lcmgl_t *lcmgl);

/* ================ OpenGL functions ===========
 *
 * These functions map directly to the OpenGL API, and all arguments should
 * be identical to their OpenGL equivalents.
 */

void lcmgl_begin(lcmgl_t *lcmgl, unsigned int glenum_mode);
void lcmgl_end(lcmgl_t *lcmgl);

void lcmgl_vertex2d(lcmgl_t *lcmgl, double v0, double v1);
void lcmgl_vertex2f(lcmgl_t *lcmgl, float v0, float v1);
void lcmgl_vertex3d(lcmgl_t *lcmgl, double v0, double v1, double v2);
void lcmgl_vertex3f(lcmgl_t *lcmgl, float v0, float v1, float v2);
void lcmgl_color3f(lcmgl_t *lcmgl, float v0, float v1, float v);
void lcmgl_color4f(lcmgl_t *lcmgl,
        float v0, float v1, float v2, float v3);
void lcmgl_normal3f(lcmgl_t *lcmgl, float v0, float v1, float v2);
void lcmgl_scalef(lcmgl_t *lcmgl, float v0, float v1, float v2);

void lcmgl_point_size(lcmgl_t *lcmgl, float v);
void lcmgl_line_width(lcmgl_t *lcmgl, float line_width);

void lcmgl_enable(lcmgl_t *lcmgl, unsigned int v);
void lcmgl_disable(lcmgl_t *lcmgl, unsigned int v);

void lcmgl_translated(lcmgl_t *lcmgl, double v0, double v1, double v2);
void lcmgl_rotated(lcmgl_t *lcmgl,
        double angle, double x, double y, double z);
void lcmgl_push_matrix(lcmgl_t * lcmgl);
void lcmgl_pop_matrix(lcmgl_t * lcmgl);
void lcmgl_load_identity(lcmgl_t *lcmgl);
void lcmgl_mult_matrixf(lcmgl_t * lcmgl, float m[16]);
void lcmgl_mult_matrixd(lcmgl_t * lcmgl, double m[16]);
void lcmgl_materialf(lcmgl_t * lcmgl, int face, int name, float c0, float c1, float c2, float c3);

void lcmgl_push_attrib(lcmgl_t *lcmgl, unsigned int attrib);
void lcmgl_pop_attrib(lcmgl_t *lcmgl);

void lcmgl_depth_func(lcmgl_t *lcmgl, unsigned int func);

// these macros provide better "work-alike" interface to GL.  They
// expect that an lcmgl* is defined in the current scope.

#define lcmglBegin(v) lcmgl_begin(lcmgl, v)
#define lcmglEnd() lcmgl_end(lcmgl)

#define lcmglVertex2d(v0, v1) lcmgl_vertex2d(lcmgl, v0, v1)
#define lcmglVertex2dv(v) lcmgl_vertex2d(lcmgl, v[0], v[1])

#define lcmglVertex2f(v0, v1) lcmgl_vertex2f(lcmgl, v0, v1)
#define lcmglVertex2fv(v) lcmgl_vertex2f(lcmgl, v[0], v[1])

#define lcmglVertex3d(v0, v1, v2) lcmgl_vertex3d(lcmgl, v0, v1, v2)
#define lcmglVertex3dv(v) lcmgl_vertex3d(lcmgl, v[0], v[1], v[2])

#define lcmglVertex3f(v0, v1, v2) lcmgl_vertex3f(lcmgl, v0, v1, v2)
#define lcmglVertex3fv(v) lcmgl_vertex3f(lcmgl, v[0], v[1], v[2])
#define lcmglNormal3f(v0, v1, v2) lcmgl_normal3f(lcmgl, v0, v1, v2)
#define lcmglNormal3fv(v) lcmgl_normal3f(lcmgl, v[0], v[1], v[2])
#define lcmglScalef(v0, v1, v2) lcmgl_scalef(lcmgl, v0, v1, v2)

#define lcmglColor3f(v0, v1, v2) lcmgl_color3f(lcmgl, v0, v1, v2)
#define lcmglColor3fv(v) lcmgl_color3f(lcmgl, v[0], v[1], v[2])

#define lcmglColor4f(v0, v1, v2, v3) lcmgl_color4f(lcmgl, v0, v1, v2, v3)
#define lcmglColor4fv(v) lcmgl_color4f(lcmgl, v[0], v[1], v[2], v[3])

#define lcmglPointSize(v) lcmgl_point_size(lcmgl, v)
#define lcmglEnable(v) lcmgl_enable(lcmgl, v)
#define lcmglDisable(v) lcmgl_disable(lcmgl, v)

#define lcmglLineWidth(size) lcmgl_line_width(lcmgl, size);

#define lcmglTranslated(v0, v1, v2) lcmgl_translated(lcmgl, v0, v1, v2)
#define lcmglTranslatef lcmglTranslated

#define lcmglRotated(angle, x, y, z) lcmgl_rotated(lcmgl, angle, x, y, z)
#define lcmglRotatef lcmglRotated

#define lcmglLoadIdentity() lcmgl_load_identity(lcmgl)

#define lcmglPushMatrix() lcmgl_push_matrix(lcmgl)
#define lcmglPopMatrix() lcmgl_pop_matrix(lcmgl)
#define lcmglMultMatrixf(m) lcmgl_mult_matrixf(lcmgl, m)
#define lcmglMultMatrixd(m) lcmgl_mult_matrixd(lcmgl, m)
#define lcmglMaterialf(face,name,c0,c1,c2) lcmgl_materialf(lcmgl, face, name, c0, c1, c2, c3);
#define lcmglMaterialfv(face,name,c) lcmgl_materialf(lcmgl, face, name, c[0], c[1], c[2], c[3]);
#define lcmglMateriald lcmglMaterialf
#define lcmglMaterialdv lcmglMaterialfv

/* ================ drawing routines not part of OpenGL ===============
 * 
 * These routines do not have a direct correspondence to the OpenGL API, but
 * are included for convenience.
 */

/**
 * lcmgl_rect:
 *
 * Draws a rectangle on the X-Y plane, centered on @xyz.
 */
void lcmgl_rect(lcmgl_t *lcmgl, double xyz[3], double size[2], 
        int filled);
/**
 * lcmgl_box:
 *
 * Draws a box, centered on @xyz.
 */
void lcmgl_box(lcmgl_t *lcmgl, double xyz[3], float size[3]);

/**
 * lcmgl_circle:
 *
 * Draws a circle on the X-Y plane, centered on @xyz.
 */
void lcmgl_circle(lcmgl_t *lcmgl, double xyz[3], double radius);

/**
 * lcmgl_sphere:
 * @see: gluSphere
 */
void lcmgl_sphere(lcmgl_t *lcmgl, double xyz[3], double radius,
        int slices, int stacks);


void lcmgl_disk(lcmgl_t *lcmgl,
        double xyz[3], double r_in, double r_out);
/**
 * lcmgl_cylinder:
 * @see: gluCylinder
 */
void lcmgl_cylinder(lcmgl_t *lcmgl,
        double base_xyz[3], double base_radius, double top_radius, double height,
        int slices, int stacks);

void lcmgl_text(lcmgl_t *lcmgl, const double xyz[3], const char *text);
void lcmgl_text_ex(lcmgl_t *lcmgl,
        const double xyz[3], const char *text, uint32_t font, uint32_t flags);


#define lcmglBox(xyz, dim) lcmgl_box(lcmgl, xyz, dim)
#define lcmglCircle(xyz, radius) lcmgl_circle(lcmgl, xyz, radius)
#define lcmglDisk(xyz, r_in, r_out) lcmgl_disk(lcmgl, xyz, r_in, r_out)

// texture API
typedef enum {
    LCMGL_LUMINANCE = 0,
    LCMGL_RGB,
    LCMGL_RGBA
} lcmgl_texture_format_t;

typedef enum {
    LCMGL_COMPRESS_NONE = 0,
} lcmgl_compress_mode_t;

/**
 * lcmgl_texture:
 *
 * Creates a texture.
 *
 * @return: the texture ID.  This ID is valid until lcmgl_switch_buffer is
 * called.
 */
int lcmgl_texture2d(lcmgl_t *lcmgl, const void *data, 
        int width, int height, int row_stride,
        lcmgl_texture_format_t format,
        lcmgl_compress_mode_t compression);

/**
 * Renders the specified texture with the active OpenGL color.
 */
void lcmgl_texture_draw_quad(lcmgl_t *lcmgl, int texture_id,
        double x_top_left,  double y_top_left,  double z_top_left,
        double x_left,  double y_left,  double z_left,
        double x_right, double y_right, double z_right,
        double x_top_right, double y_top_right, double z_top_right);

/* ======================== */

enum _lcmgl_enum_t 
{
    LCMGL_BEGIN=4,
    LCMGL_END,
    LCMGL_VERTEX3F,
    LCMGL_VERTEX3D,
    LCMGL_COLOR3F,
    LCMGL_COLOR4F,
    LCMGL_POINTSIZE,
    LCMGL_ENABLE,
    LCMGL_DISABLE,
    LCMGL_BOX,
    LCMGL_CIRCLE,
    LCMGL_LINE_WIDTH,
    LCMGL_NOP,
    LCMGL_VERTEX2D,
    LCMGL_VERTEX2F,
    LCMGL_TEXT,
    LCMGL_DISK,
    LCMGL_TRANSLATED,
    LCMGL_ROTATED,
    LCMGL_LOAD_IDENTITY,
    LCMGL_PUSH_MATRIX,
    LCMGL_POP_MATRIX,
    LCMGL_RECT,
    LCMGL_TEXT_LONG,
    LCMGL_NORMAL3F,
    LCMGL_SCALEF,
    LCMGL_MULT_MATRIXF,
    LCMGL_MULT_MATRIXD,
    LCMGL_MATERIALF,
    LCMGL_PUSH_ATTRIB,
    LCMGL_POP_ATTRIB,
    LCMGL_DEPTH_FUNC,
    LCMGL_TEX_2D,
    LCMGL_TEX_DRAW_QUAD,
    LCMGL_SPHERE,
    LCMGL_CYLINDER
};

#ifdef __cplusplus
}
#endif

#endif
