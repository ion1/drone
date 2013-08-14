#ifndef INCLUDE_BMP085_H
#define INCLUDE_BMP085_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "error-utilities.h"

typedef struct bmp085 bmp085_t;

typedef struct {
  bool have_result;
  int32_t temperature;
  int32_t pressure;
} bmp085_result_t;

bmp085_t *
bmp085_new ( const int i2c_fd, const int eoc_gpio, const int16_t oss
           , error_t *const err );

void
bmp085_free (bmp085_t *const bmp085);

void
bmp085_dump (const bmp085_t *const bmp085, FILE *const stream);

bool
bmp085_run ( bmp085_t *const bmp085, bmp085_result_t *const res
           , error_t *const err );

#endif /* INCLUDE_BMP085_H */
