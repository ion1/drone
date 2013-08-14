#ifndef INCLUDE_L3GD20_H
#define INCLUDE_L3GD20_H

#include <stdbool.h>

#include "error-utilities.h"

typedef struct l3gd20 l3gd20_t;

typedef struct {
  bool have_result;
  double x, y, z;  /* radian/s */
} l3gd20_result_t;

l3gd20_t *
l3gd20_new (const int i2c_fd, error_t *const err);

void
l3gd20_free (l3gd20_t *const l3gd20);

bool
l3gd20_run ( l3gd20_t *const l3gd20, l3gd20_result_t *const res
           , error_t *const err );

#endif /* INCLUDE_L3GD20_H */
