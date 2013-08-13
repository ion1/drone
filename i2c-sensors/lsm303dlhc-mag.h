#ifndef INCLUDE_LSM303DLHC_MAG_H
#define INCLUDE_LSM303DLHC_MAG_H

typedef struct lsm303dlhc_mag lsm303dlhc_mag_t;

lsm303dlhc_mag_t *
lsm303dlhc_mag_new (const int i2c_fd, char **errstr, int *err);

void
lsm303dlhc_mag_free (lsm303dlhc_mag_t *const mag);

bool
lsm303dlhc_mag_run ( lsm303dlhc_mag_t *const mag, double *const x_out
                   , double *const y_out, double *const z_out );

#endif /* INCLUDE_LSM303DLHC_MAG_H */
