#ifndef __bot_ppm_h__
#define __bot_ppm_h__

#include <stdio.h>
#include <inttypes.h>

/**
 * SECTION:ppm
 * @title: PPM/PGM
 * @short_description: Reading and writing PPM/PGM files
 * @include: bot2-core/bot2-core.h
 *
 * TODO
 *
 * Linking: -lbot2-core
 */

#ifdef __cplusplus
extern "C" {
#endif

int bot_ppm_read (FILE *fp, uint8_t **pixels, 
        int *width, int *height, int *rowstride);

int bot_ppm_write (FILE *fp, const uint8_t *pixels,
        int width, int height, int rowstride);

int bot_ppm_write_bottom_up (FILE *fp, uint8_t *pixels,
        int width, int height, int rowstride);

int bot_pgm_read (FILE *fp, uint8_t **pixels,
        int *width, int *height, int *rowstride);

int bot_pgm_read_fname(const char *fname, uint8_t **pixels,
        int *width, int *height, int *rowstrde);

int bot_pgm_write (FILE *fp, const uint8_t *pixels,
        int width, int height, int rowstride);

int bot_pgm_write_fname(const char *fname, const uint8_t * pixels,
        int width, int height, int rowstrde);

#ifdef __cplusplus
}
#endif

#endif
