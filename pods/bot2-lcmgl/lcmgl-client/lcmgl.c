#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>

#include "lcmtypes/lcmgl_data_t.h"


#define LCMGL_DRAW_TEXT_DROP_SHADOW      1
#define LCMGL_DRAW_TEXT_JUSTIFY_LEFT     2
#define LCMGL_DRAW_TEXT_JUSTIFY_RIGHT    4
#define LCMGL_DRAW_TEXT_JUSTIFY_CENTER   8
#define LCMGL_DRAW_TEXT_ANCHOR_LEFT     16
#define LCMGL_DRAW_TEXT_ANCHOR_RIGHT    32
#define LCMGL_DRAW_TEXT_ANCHOR_TOP      64
#define LCMGL_DRAW_TEXT_ANCHOR_BOTTOM  128
#define LCMGL_DRAW_TEXT_ANCHOR_HCENTER 256
#define LCMGL_DRAW_TEXT_ANCHOR_VCENTER 512
#define LCMGL_DRAW_TEXT_NORMALIZED_SCREEN_COORDINATES 1024
#define LCMGL_DRAW_TEXT_MONOSPACED 2048

#include "lcmgl.h"

#define INITIAL_ALLOC (1024*1024)

static int64_t _timestamp_now()
{
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return (int64_t) tv.tv_sec * 1000000 + tv.tv_usec;
}

struct lcmgl
{
    lcm_t *lcm;
    char *name;
    char *channel_name;

    int32_t scene;
    int32_t sequence;

    uint8_t *data;
    int     datalen;
    int     data_alloc;

    uint32_t texture_count;
};

union lcmgl_bytefloat
{
    uint32_t u32;
    float    f;
};

union lcmgl_bytedouble
{
    uint64_t u64;
    double   d;
};

static inline void ensure_space(lcmgl_t *lcmgl, int needed)
{
    if (lcmgl->datalen + needed < lcmgl->data_alloc)
        return;

    // grow our buffer.
    int new_alloc = lcmgl->data_alloc * 2;
    lcmgl->data = realloc(lcmgl->data, new_alloc);
    lcmgl->data_alloc = new_alloc;
}

static inline void lcmgl_encode_u8(lcmgl_t *lcmgl, uint8_t v)
{
    ensure_space(lcmgl, 1);

    lcmgl->data[lcmgl->datalen++] = v & 0xff;
}

static inline void lcmgl_encode_u32(lcmgl_t *lcmgl, uint32_t v)
{
    ensure_space(lcmgl, 4);

    lcmgl->data[lcmgl->datalen++] = (v>>24) & 0xff;
    lcmgl->data[lcmgl->datalen++] = (v>>16) & 0xff;
    lcmgl->data[lcmgl->datalen++] = (v>>8)  & 0xff;
    lcmgl->data[lcmgl->datalen++] = (v>>0)  & 0xff;
}

static inline void lcmgl_encode_u64(lcmgl_t *lcmgl, uint64_t v)
{
    ensure_space(lcmgl, 8);
    lcmgl->data[lcmgl->datalen++] = (v>>56) & 0xff;
    lcmgl->data[lcmgl->datalen++] = (v>>48) & 0xff;
    lcmgl->data[lcmgl->datalen++] = (v>>40) & 0xff;
    lcmgl->data[lcmgl->datalen++] = (v>>32) & 0xff;
    lcmgl->data[lcmgl->datalen++] = (v>>24) & 0xff;
    lcmgl->data[lcmgl->datalen++] = (v>>16) & 0xff;
    lcmgl->data[lcmgl->datalen++] = (v>>8)  & 0xff;
    lcmgl->data[lcmgl->datalen++] = (v>>0)  & 0xff;
}

static inline void lcmgl_encode_float(lcmgl_t *lcmgl, float f)
{
    union lcmgl_bytefloat u;
    ensure_space(lcmgl, 4);
    u.f = f;
    lcmgl_encode_u32(lcmgl, u.u32);
}

static inline void lcmgl_encode_double(lcmgl_t *lcmgl, double d)
{
    union lcmgl_bytedouble u;
    ensure_space(lcmgl, 8);
    u.d = d;
    lcmgl_encode_u64(lcmgl, u.u64);
}

static inline void lcmgl_encode_raw(lcmgl_t *lcmgl, int datalen, void *data)
{
    ensure_space(lcmgl, datalen);
    memcpy(lcmgl->data + lcmgl->datalen, data, datalen);
    lcmgl->datalen += datalen;
}

static inline void lcmgl_nop(lcmgl_t *lcmgl)
{
    lcmgl_encode_u8(lcmgl, LCMGL_NOP);
}

void lcmgl_switch_buffer(lcmgl_t *lcmgl)
{
    lcmgl_data_t ld;
    memset(&ld, 0, sizeof(ld));

    ld.name     = lcmgl->name;
    ld.scene    = lcmgl->scene;
    ld.sequence = lcmgl->sequence;
    ld.datalen  = lcmgl->datalen;
    ld.data     = lcmgl->data;

    lcmgl_data_t_publish(lcmgl->lcm, lcmgl->channel_name, &ld);

    lcmgl->sequence = 0;
    lcmgl->datalen = 0;

    lcmgl->texture_count = 0;

    lcmgl->scene++;
}

lcmgl_t *lcmgl_init(lcm_t *lcm, const char *name)
{
    lcmgl_t *lcmgl = (lcmgl_t*) calloc(1, sizeof(lcmgl_t));

    lcmgl->lcm = lcm;
    lcmgl->name = strdup(name);
    lcmgl->scene = _timestamp_now();
    lcmgl->channel_name = malloc(128);

    lcmgl->data = malloc(INITIAL_ALLOC);
    lcmgl->data_alloc = INITIAL_ALLOC;

    lcmgl->texture_count = 0;

    // XXX sanitize LCMGL channel name?
    snprintf(lcmgl->channel_name, 128, "LCMGL_%s", lcmgl->name);

    return lcmgl;
}

void lcmgl_destroy (lcmgl_t *lcmgl)
{
    free (lcmgl->data);
    memset (lcmgl->name, 0, strlen (lcmgl->name));
    free (lcmgl->name);
    free (lcmgl->channel_name);
    memset (lcmgl, 0, sizeof (lcmgl_t));

    free (lcmgl);
}

/* ================ OpenGL functions =========== */

void lcmgl_begin(lcmgl_t *lcmgl, unsigned int mode)
{
    lcmgl_encode_u8(lcmgl, LCMGL_BEGIN);
    lcmgl_encode_u32(lcmgl, mode);
}

void lcmgl_end(lcmgl_t *lcmgl)
{
    lcmgl_encode_u8(lcmgl, LCMGL_END);
}

void lcmgl_vertex2d(lcmgl_t *lcmgl, double v0, double v1)
{
    lcmgl_encode_u8(lcmgl, LCMGL_VERTEX2D);
    assert (isfinite (v0) && isfinite (v1));

    lcmgl_encode_double(lcmgl, v0);
    lcmgl_encode_double(lcmgl, v1);
}

void lcmgl_vertex2f(lcmgl_t *lcmgl, float v0, float v1)
{
    lcmgl_encode_u8(lcmgl, LCMGL_VERTEX2F);
    assert (isfinite (v0) && isfinite (v1));

    lcmgl_encode_float(lcmgl, v0);
    lcmgl_encode_float(lcmgl, v1);
}

void lcmgl_vertex3f(lcmgl_t *lcmgl, float v0, float v1, float v2)
{
    lcmgl_encode_u8(lcmgl, LCMGL_VERTEX3F);
    assert (isfinite (v0) && isfinite (v1) && isfinite (v2));

    lcmgl_encode_float(lcmgl, v0);
    lcmgl_encode_float(lcmgl, v1);
    lcmgl_encode_float(lcmgl, v2);
}

void lcmgl_vertex3d(lcmgl_t *lcmgl, double v0, double v1, double v2)
{
    lcmgl_encode_u8(lcmgl, LCMGL_VERTEX3D);
    assert (isfinite (v0) && isfinite (v1) && isfinite (v2));

    lcmgl_encode_double(lcmgl, v0);
    lcmgl_encode_double(lcmgl, v1);
    lcmgl_encode_double(lcmgl, v2);
}

void lcmgl_normal3f(lcmgl_t *lcmgl, float v0, float v1, float v2)
{
    lcmgl_encode_u8(lcmgl, LCMGL_NORMAL3F);
    assert (isfinite (v0) && isfinite (v1) && isfinite (v2));
    lcmgl_encode_float(lcmgl, v0);
    lcmgl_encode_float(lcmgl, v1);
    lcmgl_encode_float(lcmgl, v2);
}

void lcmgl_scalef(lcmgl_t *lcmgl, float v0, float v1, float v2)
{
    lcmgl_encode_u8(lcmgl, LCMGL_SCALEF);
    assert (isfinite (v0) && isfinite (v1) && isfinite (v2));
    lcmgl_encode_float(lcmgl, v0);
    lcmgl_encode_float(lcmgl, v1);
    lcmgl_encode_float(lcmgl, v2);
}


void lcmgl_translated(lcmgl_t *lcmgl, double v0, double v1, double v2)
{
    lcmgl_encode_u8(lcmgl, LCMGL_TRANSLATED);
    assert (isfinite (v0) && isfinite (v1) && isfinite (v2));

    lcmgl_encode_double(lcmgl, v0);
    lcmgl_encode_double(lcmgl, v1);
    lcmgl_encode_double(lcmgl, v2);
}

void lcmgl_rotated(lcmgl_t *lcmgl, double angle, double x, double y, double z)
{
    lcmgl_encode_u8(lcmgl, LCMGL_ROTATED);

    lcmgl_encode_double(lcmgl, angle);
    lcmgl_encode_double(lcmgl, x);
    lcmgl_encode_double(lcmgl, y);
    lcmgl_encode_double(lcmgl, z);
}

void lcmgl_push_matrix (lcmgl_t * lcmgl)
{
    lcmgl_encode_u8 (lcmgl, LCMGL_PUSH_MATRIX);
}

void lcmgl_pop_matrix (lcmgl_t * lcmgl)
{
    lcmgl_encode_u8 (lcmgl, LCMGL_POP_MATRIX);
}

void lcmgl_mult_matrixf(lcmgl_t *lcmgl, float m[16])
{
    lcmgl_encode_u8(lcmgl, LCMGL_MULT_MATRIXF);

    for (int i = 0; i < 16; i++)
        lcmgl_encode_float(lcmgl, m[i]);
}

void lcmgl_mult_matrixd(lcmgl_t *lcmgl, double m[16])
{
    lcmgl_encode_u8(lcmgl, LCMGL_MULT_MATRIXD);

    for (int i = 0; i < 16; i++)
        lcmgl_encode_double(lcmgl, m[i]);
}


void lcmgl_load_identity(lcmgl_t *lcmgl)
{
    lcmgl_encode_u8(lcmgl, LCMGL_LOAD_IDENTITY);
}

void lcmgl_color3f(lcmgl_t *lcmgl, float v0, float v1, float v2)
{
    lcmgl_encode_u8(lcmgl, LCMGL_COLOR3F);
    assert (isfinite (v0) && isfinite (v1) && isfinite (v2));
    lcmgl_encode_float(lcmgl, v0);
    lcmgl_encode_float(lcmgl, v1);
    lcmgl_encode_float(lcmgl, v2);
}

void lcmgl_color4f(lcmgl_t *lcmgl, float v0, float v1, float v2, float v3)
{
    lcmgl_encode_u8(lcmgl, LCMGL_COLOR4F);
    lcmgl_encode_float(lcmgl, v0);
    lcmgl_encode_float(lcmgl, v1);
    lcmgl_encode_float(lcmgl, v2);
    lcmgl_encode_float(lcmgl, v3);
}

void lcmgl_point_size(lcmgl_t *lcmgl, float v)
{
    lcmgl_encode_u8(lcmgl, LCMGL_POINTSIZE);
    lcmgl_encode_float(lcmgl, v);
}

void lcmgl_enable(lcmgl_t *lcmgl, unsigned int v)
{
    lcmgl_encode_u8(lcmgl, LCMGL_ENABLE);
    lcmgl_encode_u32(lcmgl, v);
}

void lcmgl_disable(lcmgl_t *lcmgl, unsigned int v)
{
    lcmgl_encode_u8(lcmgl, LCMGL_DISABLE);
    lcmgl_encode_u32(lcmgl, v);
}

void lcmgl_materialf(lcmgl_t *lcmgl, int face, int name, float c0,
                         float c1, float c2,float c3)
{
    lcmgl_encode_u8(lcmgl, LCMGL_MATERIALF);
    lcmgl_encode_u32(lcmgl, face);
    lcmgl_encode_u32(lcmgl, name);
    lcmgl_encode_float(lcmgl, c0);
    lcmgl_encode_float(lcmgl, c1);
    lcmgl_encode_float(lcmgl, c2);
    lcmgl_encode_float(lcmgl, c3);
}

void lcmgl_push_attrib(lcmgl_t *lcmgl, unsigned int attrib)
{
    lcmgl_encode_u8(lcmgl, LCMGL_PUSH_ATTRIB);
    lcmgl_encode_u32(lcmgl, attrib);
}

void lcmgl_pop_attrib(lcmgl_t *lcmgl)
{
    lcmgl_encode_u8(lcmgl, LCMGL_POP_ATTRIB);
}

void lcmgl_depth_func(lcmgl_t *lcmgl, unsigned int func)
{
    lcmgl_encode_u8(lcmgl, LCMGL_DEPTH_FUNC);
    lcmgl_encode_u32(lcmgl, func);
}

/* ================ drawing routines not part of OpenGL =============== */

void lcmgl_box(lcmgl_t *lcmgl, double xyz[3], float size[3])
{
    lcmgl_encode_u8(lcmgl, LCMGL_BOX);
    lcmgl_encode_double(lcmgl, xyz[0]);
    lcmgl_encode_double(lcmgl, xyz[1]);
    lcmgl_encode_double(lcmgl, xyz[2]);
    lcmgl_encode_float(lcmgl, size[0]);
    lcmgl_encode_float(lcmgl, size[1]);
    lcmgl_encode_float(lcmgl, size[2]);
}

void lcmgl_circle(lcmgl_t *lcmgl, double xyz[3], double radius)
{
    lcmgl_encode_u8(lcmgl, LCMGL_CIRCLE);
    lcmgl_encode_double(lcmgl, xyz[0]);
    lcmgl_encode_double(lcmgl, xyz[1]);
    lcmgl_encode_double(lcmgl, xyz[2]);
    lcmgl_encode_float(lcmgl, radius);
}

void lcmgl_disk(lcmgl_t *lcmgl, double xyz[3], double r_in,
        double r_out)
{
    lcmgl_encode_u8(lcmgl, LCMGL_DISK);
    lcmgl_encode_double(lcmgl, xyz[0]);
    lcmgl_encode_double(lcmgl, xyz[1]);
    lcmgl_encode_double(lcmgl, xyz[2]);
    lcmgl_encode_float(lcmgl, r_in);
    lcmgl_encode_float(lcmgl, r_out);
}

void lcmgl_cylinder(lcmgl_t *lcmgl, double base_xyz[3], double base_radius,
        double top_radius, double height, int slices, int stacks)
{
    lcmgl_encode_u8(lcmgl, LCMGL_CYLINDER);
    lcmgl_encode_double(lcmgl, base_xyz[0]);
    lcmgl_encode_double(lcmgl, base_xyz[1]);
    lcmgl_encode_double(lcmgl, base_xyz[2]);
    lcmgl_encode_double(lcmgl, base_radius);
    lcmgl_encode_double(lcmgl, top_radius);
    lcmgl_encode_double(lcmgl, height);
    lcmgl_encode_u32(lcmgl, slices);
    lcmgl_encode_u32(lcmgl, stacks);
}

void lcmgl_sphere(lcmgl_t *lcmgl, double xyz[3], double radius, 
        int slices, int stacks)
{
    lcmgl_encode_u8(lcmgl, LCMGL_SPHERE);
    lcmgl_encode_double(lcmgl, xyz[0]);
    lcmgl_encode_double(lcmgl, xyz[1]);
    lcmgl_encode_double(lcmgl, xyz[2]);
    lcmgl_encode_double(lcmgl, radius);
    lcmgl_encode_u32(lcmgl, slices);
    lcmgl_encode_u32(lcmgl, stacks);
}

void lcmgl_line_width(lcmgl_t *lcmgl, float line_width)
{
    lcmgl_encode_u8(lcmgl, LCMGL_LINE_WIDTH);
    lcmgl_encode_float(lcmgl, line_width);
}

void lcmgl_text_ex(lcmgl_t *lcmgl, const double xyz[3],
        const char *text, uint32_t font, uint32_t flags)
{
    lcmgl_encode_u8(lcmgl, LCMGL_TEXT_LONG);
    lcmgl_encode_u32(lcmgl, font);
    lcmgl_encode_u32(lcmgl, flags);

    lcmgl_encode_double(lcmgl, xyz[0]);
    lcmgl_encode_double(lcmgl, xyz[1]);
    lcmgl_encode_double(lcmgl, xyz[2]);

    int len = strlen(text);

    lcmgl_encode_u32(lcmgl, len);
    for (int i = 0; i < len; i++)
        lcmgl_encode_u8(lcmgl, text[i]);
}

void lcmgl_text(lcmgl_t *lcmgl, const double xyz[3], const char *text)
{
    lcmgl_text_ex(lcmgl, xyz, text, 0,
                 LCMGL_DRAW_TEXT_DROP_SHADOW |
                 LCMGL_DRAW_TEXT_JUSTIFY_CENTER |
                 LCMGL_DRAW_TEXT_ANCHOR_HCENTER |
                 LCMGL_DRAW_TEXT_ANCHOR_VCENTER);
}

////////// vertex buffer
//lcmgl_vertex_buffer_t *lcmgl_vertex_buffer_create(int capacity,
//        lcmgl_vertex_buffer_full_callback_t full_callback, void *user)
//{
//    lcmgl_vertex_buffer_t *vertbuf = (lcmgl_vertex_buffer_t*) calloc(1, sizeof(lcmgl_vertex_buffer_t));
//    vertbuf->size = 0;
//    vertbuf->capacity = capacity;
//    vertbuf->vertices = (struct lcmgl_vertex*) calloc(capacity,
//            sizeof(struct lcmgl_vertex));
//    vertbuf->full_callback = full_callback;
//    vertbuf->user = user;
//
//    return vertbuf;
//}
//
//void lcmgl_vertex_buffer_destroy(lcmgl_vertex_buffer_t *vertbuf)
//{
//    free(vertbuf->vertices);
//    free(vertbuf);
//}
//
//void lcmgl_vertex_buffer_flush(lcmgl_vertex_buffer_t *vertbuf)
//{
//    if (vertbuf->size)
//        vertbuf->full_callback(vertbuf, vertbuf->user);
//}
//
//void lcmgl_vertex_buffer_add(lcmgl_vertex_buffer_t *vertbuf, double v[3])
//{
//    assert(vertbuf->size < vertbuf->capacity);
//
//    memcpy(vertbuf->vertices[vertbuf->size].xyz, v, sizeof(double)*3);
//    vertbuf->size++;
//    if (vertbuf->size == vertbuf->capacity)
//        lcmgl_vertex_buffer_flush(vertbuf);
//}
//
//void lcmgl_vertex_buffer_send(lcmgl_vertex_buffer_t *vertbuf, lcmgl_t *lcmgl)
//{
//    for (int i = 0; i < vertbuf->size; i++) {
///*        printf("%f %f %f\n", vertbuf->vertices[i].xyz[0],
//               vertbuf->vertices[i].xyz[1],
//               vertbuf->vertices[i].xyz[2]);*/
//        lcmglVertex3dv(vertbuf->vertices[i].xyz);
//    }
//}

void lcmgl_rect(lcmgl_t *lcmgl, double xyz[3], double size[2], int filled)
{
    lcmgl_encode_u8(lcmgl, LCMGL_RECT);

    lcmgl_encode_double(lcmgl, xyz[0]);
    lcmgl_encode_double(lcmgl, xyz[1]);
    lcmgl_encode_double(lcmgl, xyz[2]);

    lcmgl_encode_double(lcmgl, size[0]);
    lcmgl_encode_double(lcmgl, size[1]);

    lcmgl_encode_u8(lcmgl, filled);
}

// texture API

int 
lcmgl_texture2d(lcmgl_t *lcmgl, const void *data, 
        int width, int height, int row_stride,
        lcmgl_texture_format_t format,
        lcmgl_compress_mode_t compression)
{
    lcmgl_encode_u8(lcmgl, LCMGL_TEX_2D);

    uint32_t tex_id = lcmgl->texture_count + 1;
    lcmgl->texture_count ++;

    lcmgl_encode_u32(lcmgl, tex_id);

    int bytes_per_pixel = 1;
    switch(format) {
        case LCMGL_LUMINANCE:
            bytes_per_pixel = 1;
            break;
        case LCMGL_RGB:
            bytes_per_pixel = 3;
            break;
        case LCMGL_RGBA:
            bytes_per_pixel = 4;
            break;
    }
    int bytes_per_row = width * bytes_per_pixel;
    int datalen = bytes_per_row * height;

    lcmgl_encode_u32(lcmgl, width);
    lcmgl_encode_u32(lcmgl, height);
    lcmgl_encode_u32(lcmgl, format);
    lcmgl_encode_u32(lcmgl, compression);

    switch(compression) {
        case LCMGL_COMPRESS_NONE:
            lcmgl_encode_u32(lcmgl, datalen);
            for(int row=0; row<height; row++) {
                void *row_start = (uint8_t*)data + row * row_stride;
                lcmgl_encode_raw(lcmgl, bytes_per_row, row_start);
            }
            break;
    }

    return tex_id;
}

/**
 * Renders the specified texture with the active OpenGL color.
 */
void 
lcmgl_texture_draw_quad(lcmgl_t *lcmgl, int texture_id,
        double x_top_left,  double y_top_left,  double z_top_left,
        double x_left,  double y_left,  double z_left,
        double x_right, double y_right, double z_right,
        double x_top_right, double y_top_right, double z_top_right)
{
    if(texture_id > lcmgl->texture_count || texture_id <= 0) {
        fprintf(stderr, "%s -- WARNING: invalid texture_id %d\n", __FUNCTION__, texture_id);
        return;
    }

    lcmgl_encode_u8(lcmgl, LCMGL_TEX_DRAW_QUAD);
    lcmgl_encode_u32(lcmgl, texture_id);

    lcmgl_encode_double(lcmgl, x_top_left);
    lcmgl_encode_double(lcmgl, y_top_left);
    lcmgl_encode_double(lcmgl, z_top_left);

    lcmgl_encode_double(lcmgl, x_left);
    lcmgl_encode_double(lcmgl, y_left);
    lcmgl_encode_double(lcmgl, z_left);

    lcmgl_encode_double(lcmgl, x_right);
    lcmgl_encode_double(lcmgl, y_right);
    lcmgl_encode_double(lcmgl, z_right);

    lcmgl_encode_double(lcmgl, x_top_right);
    lcmgl_encode_double(lcmgl, y_top_right);
    lcmgl_encode_double(lcmgl, z_top_right);
}
