#ifndef INCLUDE_I2C_SENSORS_H
#define INCLUDE_I2C_SENSORS_H

#include <stdbool.h>
#include <stdio.h>

#include "error-utilities.h"

typedef struct i2c_sensors i2c_sensors_t;

i2c_sensors_t *
i2c_sensors_new ( const char *const dev, const int bmp085_eoc_gpio
                , error_t *const err );

void
i2c_sensors_free (i2c_sensors_t *const sensors);

void
i2c_sensors_dump (const i2c_sensors_t *const sensors, FILE *const stream);

bool
i2c_sensors_run (i2c_sensors_t *const sensors, error_t *const err);

#endif /* INCLUDE_I2C_SENSORS_H */
