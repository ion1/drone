#ifndef INCLUDE_I2C_SENSORS_H
#define INCLUDE_I2C_SENSORS_H

#include <stdbool.h>
#include <stdio.h>

#include "bmp085.h"
#include "error-utilities.h"
#include "l3gd20.h"
#include "lsm303dlhc-acc.h"
#include "lsm303dlhc-mag.h"

typedef struct i2c_sensors i2c_sensors_t;

typedef struct {
  bmp085_result_t baro;
  l3gd20_result_t gyro;
  lsm303dlhc_acc_result_t acc;
  lsm303dlhc_mag_result_t mag;
} i2c_sensors_result_t;

i2c_sensors_t *
i2c_sensors_new ( const char *const dev, const int bmp085_eoc_gpio
                , error_t *const err );

void
i2c_sensors_free (i2c_sensors_t *const sensors);

void
i2c_sensors_dump (const i2c_sensors_t *const sensors, FILE *const stream);

bool
i2c_sensors_run ( i2c_sensors_t *const sensors, i2c_sensors_result_t *const res
                , error_t *const err);

#endif /* INCLUDE_I2C_SENSORS_H */
