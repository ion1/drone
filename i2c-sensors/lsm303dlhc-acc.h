#ifndef INCLUDE_LSM303DLHC_ACC_H
#define INCLUDE_LSM303DLHC_ACC_H

#include <stdbool.h>

#include "error-utilities.h"

typedef struct lsm303dlhc_acc lsm303dlhc_acc_t;

typedef struct {
  bool have_result;
  double x, y, z;  /* m/s² */
} lsm303dlhc_acc_result_t;

lsm303dlhc_acc_t *
lsm303dlhc_acc_new (const int i2c_fd, error_t *const err);

void
lsm303dlhc_acc_free (lsm303dlhc_acc_t *const acc);

bool
lsm303dlhc_acc_run ( lsm303dlhc_acc_t *const acc
                   , lsm303dlhc_acc_result_t *const res, error_t *const err );

#endif /* INCLUDE_LSM303DLHC_ACC_H */
