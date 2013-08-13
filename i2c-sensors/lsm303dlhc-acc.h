#ifndef INCLUDE_LSM303DLHC_ACC_H
#define INCLUDE_LSM303DLHC_ACC_H

#include <stdbool.h>

typedef struct lsm303dlhc_acc lsm303dlhc_acc_t;

lsm303dlhc_acc_t *
lsm303dlhc_acc_new (const int i2c_fd, char **errstr, int *err);

void
lsm303dlhc_acc_free (lsm303dlhc_acc_t *const acc);

bool
lsm303dlhc_acc_run ( lsm303dlhc_acc_t *const acc, double *const x_out
                   , double *const y_out, double *const z_out );

#endif /* INCLUDE_LSM303DLHC_ACC_H */
