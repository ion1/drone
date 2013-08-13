#ifndef INCLUDE_I2C_SENSORS_H
#define INCLUDE_I2C_SENSORS_H

#include <stdio.h>

typedef struct i2c_sensors i2c_sensors_t;

i2c_sensors_t *
i2c_sensors_new ( const char *const dev, const int bmp085_eoc_gpio
                , char **errstr, int *err );
void
i2c_sensors_free (i2c_sensors_t *const sensors);

void
i2c_sensors_dump (const i2c_sensors_t *const sensors, FILE *const stream);

void
i2c_sensors_run (i2c_sensors_t *const sensors);

#endif /* INCLUDE_I2C_SENSORS_H */
