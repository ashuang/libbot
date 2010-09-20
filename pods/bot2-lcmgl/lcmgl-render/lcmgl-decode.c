/*
 * decodes and renders LCMGL data
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include <bot2-core/fasttrig.h>
#include <bot2-vis/bot2-vis.h>

#include <lcmtypes/lcmgl_data_t.h>
#include "../lcmgl-client/lcmgl.h"
#include "lcmgl-decode.h"

union fu32
{
    float    f;
    uint32_t u32;
};

union du64
{
    double   d;
    uint64_t u64;
};

typedef struct lcmgl_decoder lcmgl_decoder_t;
struct lcmgl_decoder
{
    uint8_t *data;
    int      datalen;
    int      datapos;
};


static inline uint8_t lcmgl_decode_u8(lcmgl_decoder_t *ldec)
{
    return ldec->data[ldec->datapos++];
}

static inline uint32_t lcmgl_decode_u32(lcmgl_decoder_t *ldec)
{
    uint32_t v = 0;
    v += ldec->data[ldec->datapos++]<<24;
    v += ldec->data[ldec->datapos++]<<16;
    v += ldec->data[ldec->datapos++]<<8;
    v += ldec->data[ldec->datapos++]<<0;
    return v;
}

static inline uint64_t lcmgl_decode_u64(lcmgl_decoder_t *ldec)
{
    uint64_t v = 0;
    v += (uint64_t)(ldec->data[ldec->datapos++])<<56;
    v += (uint64_t)(ldec->data[ldec->datapos++])<<48;
    v += (uint64_t)(ldec->data[ldec->datapos++])<<40;
    v += (uint64_t)(ldec->data[ldec->datapos++])<<32;
    v += (uint64_t)(ldec->data[ldec->datapos++])<<24;
    v += (uint64_t)(ldec->data[ldec->datapos++])<<16;
    v += (uint64_t)(ldec->data[ldec->datapos++])<<8;
    v += (uint64_t)(ldec->data[ldec->datapos++])<<0;
    return v;
}

//static inline void lcmgl_decode_raw(lcmgl_decoder_t *ldec, int datalen, void *result)
//{
//    memcpy(result, ldec->datapos, datalen);
//    ldec->datapos += datalen;
//}

static inline float lcmgl_decode_float(lcmgl_decoder_t *ldec)
{
    union fu32 u;
    u.u32 = lcmgl_decode_u32(ldec);
    return u.f;
}

static inline double lcmgl_decode_double(lcmgl_decoder_t *ldec)
{
    union du64 u;
    u.u64 = lcmgl_decode_u64(ldec);
    return u.d;
}

static void gl_box(double xyz[3], double dim[3])
{
    glBegin(GL_QUADS);

    glNormal3f(-1, 0, 0);
    glVertex3f(xyz[0]-dim[0]/2, xyz[1]-dim[1]/2, xyz[2]-dim[2]/2);
    glVertex3f(xyz[0]-dim[0]/2, xyz[1]-dim[1]/2, xyz[2]+dim[2]/2);
    glVertex3f(xyz[0]-dim[0]/2, xyz[1]+dim[1]/2, xyz[2]+dim[2]/2);
    glVertex3f(xyz[0]-dim[0]/2, xyz[1]+dim[1]/2, xyz[2]-dim[2]/2);

    glNormal3f(1, 0, 0);
    glVertex3f(xyz[0]+dim[0]/2, xyz[1]-dim[1]/2, xyz[2]-dim[2]/2);
    glVertex3f(xyz[0]+dim[0]/2, xyz[1]-dim[1]/2, xyz[2]+dim[2]/2);
    glVertex3f(xyz[0]+dim[0]/2, xyz[1]+dim[1]/2, xyz[2]+dim[2]/2);
    glVertex3f(xyz[0]+dim[0]/2, xyz[1]+dim[1]/2, xyz[2]-dim[2]/2);

    glNormal3f(0,0,-1);
    glVertex3f(xyz[0]-dim[0]/2, xyz[1]-dim[1]/2, xyz[2]-dim[2]/2);
    glVertex3f(xyz[0]-dim[0]/2, xyz[1]+dim[1]/2, xyz[2]-dim[2]/2);
    glVertex3f(xyz[0]+dim[0]/2, xyz[1]+dim[1]/2, xyz[2]-dim[2]/2);
    glVertex3f(xyz[0]+dim[0]/2, xyz[1]-dim[1]/2, xyz[2]-dim[2]/2);

    glNormal3f(0,0,1);
    glVertex3f(xyz[0]-dim[0]/2, xyz[1]-dim[1]/2, xyz[2]+dim[2]/2);
    glVertex3f(xyz[0]-dim[0]/2, xyz[1]+dim[1]/2, xyz[2]+dim[2]/2);
    glVertex3f(xyz[0]+dim[0]/2, xyz[1]+dim[1]/2, xyz[2]+dim[2]/2);
    glVertex3f(xyz[0]+dim[0]/2, xyz[1]-dim[1]/2, xyz[2]+dim[2]/2);

    glNormal3f(0,-1,0);
    glVertex3f(xyz[0]-dim[0]/2, xyz[1]-dim[1]/2, xyz[2]-dim[2]/2);
    glVertex3f(xyz[0]-dim[0]/2, xyz[1]-dim[1]/2, xyz[2]+dim[2]/2);
    glVertex3f(xyz[0]+dim[0]/2, xyz[1]-dim[1]/2, xyz[2]+dim[2]/2);
    glVertex3f(xyz[0]+dim[0]/2, xyz[1]-dim[1]/2, xyz[2]-dim[2]/2);

    glNormal3f(0,1,0);
    glVertex3f(xyz[0]-dim[0]/2, xyz[1]+dim[1]/2, xyz[2]-dim[2]/2);
    glVertex3f(xyz[0]-dim[0]/2, xyz[1]+dim[1]/2, xyz[2]+dim[2]/2);
    glVertex3f(xyz[0]+dim[0]/2, xyz[1]+dim[1]/2, xyz[2]+dim[2]/2);
    glVertex3f(xyz[0]+dim[0]/2, xyz[1]+dim[1]/2, xyz[2]-dim[2]/2);

    glEnd();
}

typedef struct {
    int lcmgl_tex_id;
    BotGlTexture *tex;
} _lcmgl_texture_t;

void lcmgl_decode(uint8_t *data, int datalen)
{
    lcmgl_decoder_t ldec;
    ldec.data = data;
    ldec.datalen = datalen;
    ldec.datapos = 0;

    _lcmgl_texture_t **textures = NULL;
    int ntextures = 0;

    while (ldec.datapos < ldec.datalen) {

        uint8_t opcode = lcmgl_decode_u8(&ldec);
        switch (opcode) {

        case LCMGL_BEGIN:
        {
            uint32_t v = lcmgl_decode_u32(&ldec);
            glBegin(v);
            break;
        }

        case LCMGL_END:
            glEnd();
            break;

        case LCMGL_VERTEX2D:
        {
            double v[2];
            for (int i = 0; i < 2; i++)
                v[i] = lcmgl_decode_double(&ldec);

            glVertex2dv(v);
            break;
        }

        case LCMGL_VERTEX2F:
        {
            float v[2];
            for (int i = 0; i < 2; i++)
                v[i] = lcmgl_decode_float(&ldec);

            glVertex2fv(v);
            break;
        }

        case LCMGL_VERTEX3F:
        {
            float v[3];
            for (int i = 0; i < 3; i++)
                v[i] = lcmgl_decode_float(&ldec);

            glVertex3fv(v);
            break;
        }

        case LCMGL_VERTEX3D:
        {
            double v[3];
            for (int i = 0; i < 3; i++)
                v[i] = lcmgl_decode_double(&ldec);

            glVertex3dv(v);
            break;
        }

        case LCMGL_NORMAL3F:
        {
            float v[3];
            for (int i = 0; i < 3; i++)
                v[i] = lcmgl_decode_float(&ldec);

            glNormal3fv(v);
            break;
        }

        case LCMGL_TRANSLATED:
        {
            double v[3];
            for (int i = 0; i < 3; i++)
                v[i] = lcmgl_decode_double(&ldec);

            glTranslated(v[0], v[1], v[2]);
            break;
        }

        case LCMGL_ROTATED:
        {
            double theta = lcmgl_decode_double(&ldec);

            double v[3];
            for (int i = 0; i < 3; i++)
                v[i] = lcmgl_decode_double(&ldec);

            glRotated(theta, v[0], v[1], v[2]);
            break;
        }

        case LCMGL_LOAD_IDENTITY:
        {
            glLoadIdentity();
            break;
        }

        case LCMGL_PUSH_MATRIX:
        {
            glPushMatrix ();
            break;
        }

        case LCMGL_POP_MATRIX:
        {
            glPopMatrix ();
            break;
        }

        case LCMGL_MULT_MATRIXF:
        {
            float m[16];
            for (int i = 0; i < 16; i++)
                m[i] = lcmgl_decode_float(&ldec);

            glMultMatrixf(m);
            break;
        }
        case LCMGL_MULT_MATRIXD:
        {
            double m[16];
            for (int i = 0; i < 16; i++)
                m[i] = lcmgl_decode_double(&ldec);

            glMultMatrixd(m);
            break;
        }

        case LCMGL_SCALEF:
        {
            float v[3];
            for (int i = 0; i < 3; i++)
                v[i] = lcmgl_decode_float(&ldec);

            glScalef(v[0],v[1],v[2]);
            break;
        }

        case LCMGL_COLOR3F:
        {
            float v[3];
            for (int i = 0; i < 3; i++)
                v[i] = lcmgl_decode_float(&ldec);

            glColor3fv(v);
            break;
        }

        case LCMGL_COLOR4F:
        {
            float v[4];
            for (int i = 0; i < 4; i++)
                v[i] = lcmgl_decode_float(&ldec);

            glColor4fv(v);
            break;
        }

        case LCMGL_POINTSIZE:
            glPointSize(lcmgl_decode_float(&ldec));
            break;

        case LCMGL_LINE_WIDTH:
            glLineWidth(lcmgl_decode_float(&ldec));
            break;

        case LCMGL_ENABLE:
            glEnable(lcmgl_decode_u32(&ldec));
            break;

        case LCMGL_DISABLE:
            glDisable(lcmgl_decode_u32(&ldec));
            break;

        case LCMGL_NOP:
            break;

        case LCMGL_PUSH_ATTRIB:
        {
          uint32_t v = lcmgl_decode_u32(&ldec);
          glPushAttrib(v);
          break;
        }

        case LCMGL_POP_ATTRIB:
        {
          glPopAttrib();
          break;
        }

        case LCMGL_DEPTH_FUNC:
        {
          uint32_t v = lcmgl_decode_u32(&ldec);
          glDepthFunc(v);

          break;
        }


        case LCMGL_BOX:
        {
            double xyz[3];
            for (int i = 0; i < 3; i++)
                xyz[i] = lcmgl_decode_double(&ldec);
            double dim[3];
            for (int i = 0; i < 3; i++)
                dim[i] = lcmgl_decode_float(&ldec);

            gl_box(xyz, dim);
            break;
        }

        case LCMGL_RECT:
        {
            double xyz[3], size[2];

            for (int i = 0; i < 3; i++)
                xyz[i] = lcmgl_decode_double(&ldec);
            for (int i = 0; i < 2; i++)
                size[i] = lcmgl_decode_double(&ldec);
            int filled = lcmgl_decode_u8(&ldec);

            glPushMatrix();
            glTranslated(xyz[0], xyz[1], xyz[2]);
            if (filled)
                glBegin(GL_QUADS);
            else
                glBegin(GL_LINE_LOOP);
            glVertex3d(-size[0]/2, -size[1]/2, xyz[2]);
            glVertex3d(-size[0]/2,  size[1]/2, xyz[2]);
            glVertex3d( size[0]/2,  size[1]/2, xyz[2]);
            glVertex3d( size[0]/2, -size[1]/2, xyz[2]);
            glEnd();
            glPopMatrix();
            break;
        }

        case LCMGL_CIRCLE:
        {
            double xyz[3];

            for (int i = 0; i < 3; i++)
                xyz[i] = lcmgl_decode_double(&ldec);
            float radius = lcmgl_decode_float(&ldec);

            glBegin(GL_LINE_STRIP);
            int segments = 40;

            for (int i = 0; i <= segments; i++) {
                double s,c;
                bot_fasttrig_sincos(2*M_PI/segments * i, &s, &c);
                glVertex3d(xyz[0] + c*radius, xyz[1] + s*radius, xyz[2]);
            }

            glEnd();
            break;
        }

        case LCMGL_SPHERE:
        {
            double xyz[3];
            for (int i = 0; i < 3; i++)
                xyz[i] = lcmgl_decode_double(&ldec);
            double radius = lcmgl_decode_double(&ldec);
            int slices = lcmgl_decode_u32(&ldec);
            int stacks = lcmgl_decode_u32(&ldec);

            glPushAttrib(GL_ENABLE_BIT);
            glEnable(GL_DEPTH_TEST);
            glPushMatrix();
            glTranslatef(xyz[0], xyz[1], xyz[2]);
            GLUquadricObj *q = gluNewQuadric();
            gluSphere(q, radius, slices, stacks);
            gluDeleteQuadric(q);
            glPopMatrix();
            glPopAttrib();
            break;
        }

        case LCMGL_DISK:
        {
            double xyz[3];

            for (int i = 0; i < 3; i++)
                xyz[i] = lcmgl_decode_double(&ldec);
            float r_in = lcmgl_decode_float(&ldec);
            float r_out = lcmgl_decode_float(&ldec);

            GLUquadricObj *q = gluNewQuadric();
            glPushMatrix();
            glTranslatef(xyz[0], xyz[1], xyz[2]);
            gluDisk(q, r_in, r_out, 15, 1);
            glPopMatrix();
            gluDeleteQuadric(q);
            break;
        }

        case LCMGL_CYLINDER:
        {
            double base_xyz[3];
            for (int i = 0; i < 3; i++)
                base_xyz[i] = lcmgl_decode_double(&ldec);
            double r_base = lcmgl_decode_double(&ldec);
            double r_top = lcmgl_decode_double(&ldec);
            double height = lcmgl_decode_double(&ldec);
            int slices = lcmgl_decode_u32(&ldec);
            int stacks = lcmgl_decode_u32(&ldec);

            glPushAttrib(GL_ENABLE_BIT);
            glEnable(GL_DEPTH_TEST);
            glPushMatrix();
            glTranslatef(base_xyz[0], base_xyz[1], base_xyz[2]);
            GLUquadricObj *q = gluNewQuadric();
            gluCylinder(q, r_base, r_top, height, slices, stacks);
            glPopMatrix();
            gluDeleteQuadric(q);
            glPopAttrib();
            break;
        }

        case LCMGL_TEXT:
        {
            int font = lcmgl_decode_u8(&ldec);
            int flags = lcmgl_decode_u8(&ldec);

            (void) font; (void) flags;
            double xyz[3];
            for (int i = 0; i < 3; i++)
                xyz[i] = lcmgl_decode_double(&ldec);

            int len = lcmgl_decode_u32(&ldec);
            char buf[len+1];
            for (int i = 0; i < len; i++)
                buf[i] = lcmgl_decode_u8(&ldec);
            buf[len] = 0;

            bot_gl_draw_text(xyz, NULL, buf, 0);
            break;
        }

        case LCMGL_TEXT_LONG:
        {
            uint32_t font = lcmgl_decode_u32(&ldec);
            uint32_t flags = lcmgl_decode_u32(&ldec);

            (void) font;

            double xyz[3];
            for (int i = 0; i < 3; i++)
                xyz[i] = lcmgl_decode_double(&ldec);

            int len = lcmgl_decode_u32(&ldec);
            char buf[len+1];
            for (int i = 0; i < len; i++)
                buf[i] = lcmgl_decode_u8(&ldec);
            buf[len] = 0;

            bot_gl_draw_text(xyz, NULL, buf, flags);
            break;
        }
        case LCMGL_MATERIALF:
        {
            int face = lcmgl_decode_u32(&ldec);
            int name = lcmgl_decode_u32(&ldec);
            float c[4];
            for (int i = 0; i < 4; i++)
                c[i] = lcmgl_decode_float(&ldec);
            glMaterialfv(face,name,c);
            break;
        }
        case LCMGL_TEX_2D:
        {
            int id = lcmgl_decode_u32(&ldec);
            int width = lcmgl_decode_u32(&ldec);
            int height = lcmgl_decode_u32(&ldec);
            int format = lcmgl_decode_u32(&ldec);
            int compression = lcmgl_decode_u32(&ldec);

            int raw_datalen = lcmgl_decode_u32(&ldec);
            void *data_uncopressed = NULL;
            if(LCMGL_COMPRESS_NONE == compression) {
                data_uncopressed = &ldec.data[ldec.datapos];
                ldec.datapos += raw_datalen;
            }

            int bytes_per_pixel = 1;
            GLenum gl_format = GL_LUMINANCE;
            switch(format) {
                case LCMGL_LUMINANCE:
                    bytes_per_pixel = 1;
                    gl_format = GL_LUMINANCE;
                    break;
                case LCMGL_RGB:
                    bytes_per_pixel = 3;
                    gl_format = GL_RGB;
                    break;
                case LCMGL_RGBA:
                    bytes_per_pixel = 4;
                    gl_format = GL_RGBA;
                    break;
            }
            int bytes_per_row = width * bytes_per_pixel;
            int max_data_size = height * bytes_per_row;

            _lcmgl_texture_t *tex = (_lcmgl_texture_t*)malloc(sizeof(_lcmgl_texture_t));

            tex->tex = bot_gl_texture_new(width, height, max_data_size);
            GLenum gl_type = GL_UNSIGNED_BYTE;
            bot_gl_texture_upload(tex->tex, gl_format, gl_type,
                    bytes_per_row, data_uncopressed);

            ntextures++;
            textures = realloc(textures, ntextures * sizeof(_lcmgl_texture_t));
            textures[ntextures-1] = tex;

            if(id != ntextures) {
                // TODO emit warning...
            }
            break;
        }
        case LCMGL_TEX_DRAW_QUAD:
        {
            int id = lcmgl_decode_u32(&ldec);

            double x_top_left = lcmgl_decode_double(&ldec);
            double y_top_left = lcmgl_decode_double(&ldec);
            double z_top_left = lcmgl_decode_double(&ldec);

            double x_top_right = lcmgl_decode_double(&ldec);
            double y_top_right = lcmgl_decode_double(&ldec);
            double z_top_right = lcmgl_decode_double(&ldec);

            double x_bot_right = lcmgl_decode_double(&ldec);
            double y_bot_right = lcmgl_decode_double(&ldec);
            double z_bot_right = lcmgl_decode_double(&ldec);

            double x_bot_left = lcmgl_decode_double(&ldec);
            double y_bot_left = lcmgl_decode_double(&ldec);
            double z_bot_left = lcmgl_decode_double(&ldec);

            if(id <= ntextures) {
                _lcmgl_texture_t *tex = textures[id - 1];
                bot_gl_texture_draw_coords(tex->tex, 
                        x_top_left, y_top_left, z_top_left,
                        x_top_right, y_top_right, z_top_right,
                        x_bot_right, y_bot_right, z_bot_right,
                        x_bot_left, y_bot_left, z_bot_left);
            }
            break;
        }
        default:
            printf("lcmgl unknown opcode %d\n", opcode);
            break;
        }
    }

    for(int i=0; i<ntextures; i++) {
        bot_gl_texture_free(textures[i]->tex);
        free(textures[i]);
    }
    free(textures);
}

