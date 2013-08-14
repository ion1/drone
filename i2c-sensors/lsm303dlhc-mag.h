#ifndef INCLUDE_LSM303DLHC_MAG_H
#define INCLUDE_LSM303DLHC_MAG_H

#include <stdbool.h>

#include "error-utilities.h"

typedef struct lsm303dlhc_mag lsm303dlhc_mag_t;

typedef struct {
  bool have_result;
  double x, y, z;  /* T */
} lsm303dlhc_mag_result_t;

lsm303dlhc_mag_t *
lsm303dlhc_mag_new (const int i2c_fd, error_t *const err);

void
lsm303dlhc_mag_free (lsm303dlhc_mag_t *const mag);

bool
lsm303dlhc_mag_run ( lsm303dlhc_mag_t *const mag
                   , lsm303dlhc_mag_result_t *const res, error_t *const err);

#endif /* INCLUDE_LSM303DLHC_MAG_H */
