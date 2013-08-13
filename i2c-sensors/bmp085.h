#ifndef INCLUDE_BMP085_H
#define INCLUDE_BMP085_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct bmp085 bmp085_t;

bmp085_t *
bmp085_new ( const int i2c_fd, const int eoc_gpio, const int16_t oss
           , char **errstr, int *err );

void
bmp085_free (bmp085_t *const bmp085);

void
bmp085_dump (const bmp085_t *const bmp085, FILE *const stream);

bool
bmp085_run ( bmp085_t *const bmp085
           , int32_t *const t_out, int32_t *const p_out );

#endif /* INCLUDE_BMP085_H */
