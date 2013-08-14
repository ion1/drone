#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "i2c-sensors.h"

#include "bmp085.h"
#include "common.h"
#include "error-utilities.h"
#include "l3gd20.h"
#include "lsm303dlhc-acc.h"
#include "lsm303dlhc-mag.h"

struct i2c_sensors {
  int fd;
  bmp085_t *bmp085;
  l3gd20_t *l3gd20;
  lsm303dlhc_acc_t *lsm303dlhc_acc;
  lsm303dlhc_mag_t *lsm303dlhc_mag;
};

i2c_sensors_t *
i2c_sensors_new ( const char *const dev, const int bmp085_eoc_gpio
                , error_t *const err )
{
  i2c_sensors_t *sensors = malloc (sizeof (i2c_sensors_t));
  if (! sensors) {
    error_strerror (err, errno);
    error_prefix (err, "malloc failed");
    goto malloc_failed;
  }

  if ((sensors->fd = open (dev, O_RDWR)) < 0) {
    error_strerror (err, errno);
    error_prefix_printf (err, "open %s failed", dev);
    goto open_failed;
  }

  if (! (sensors->bmp085 = bmp085_new (sensors->fd, bmp085_eoc_gpio, 3, err)))
    goto bmp085_failed;

  if (! (sensors->l3gd20 = l3gd20_new (sensors->fd, err)))
    goto l3gd20_failed;

  if (! (sensors->lsm303dlhc_acc =
           lsm303dlhc_acc_new (sensors->fd, err)))
    goto lsm303dlhc_acc_failed;

  if (! (sensors->lsm303dlhc_mag =
           lsm303dlhc_mag_new (sensors->fd, err)))
    goto lsm303dlhc_mag_failed;

  return sensors;

/* everything_failed: */
  lsm303dlhc_mag_free (sensors->lsm303dlhc_mag);

lsm303dlhc_mag_failed:
  lsm303dlhc_acc_free (sensors->lsm303dlhc_acc);

lsm303dlhc_acc_failed:
  l3gd20_free (sensors->l3gd20);

l3gd20_failed:
  bmp085_free (sensors->bmp085);

bmp085_failed:
  close (sensors->fd);

open_failed:
  free (sensors);

malloc_failed:
  error_prefix (err, "i2c_sensors_new");
  return NULL;
}

void
i2c_sensors_free (i2c_sensors_t *const sensors)
{
  bmp085_free (sensors->bmp085);
  sensors->bmp085 = (bmp085_t *)POISON;

  l3gd20_free (sensors->l3gd20);
  sensors->l3gd20 = (l3gd20_t *)POISON;

  lsm303dlhc_acc_free (sensors->lsm303dlhc_acc);
  sensors->lsm303dlhc_acc = (lsm303dlhc_acc_t *)POISON;

  lsm303dlhc_mag_free (sensors->lsm303dlhc_mag);
  sensors->lsm303dlhc_mag = (lsm303dlhc_mag_t *)POISON;

  close (sensors->fd);
  sensors->fd = POISON;

  free (sensors);
}

void
i2c_sensors_dump (const i2c_sensors_t *const sensors, FILE *const stream)
{
  bmp085_dump (sensors->bmp085, stream);
}

bool
i2c_sensors_run (i2c_sensors_t *const sensors, error_t *const err)
{
  bmp085_result_t baro_res;
  l3gd20_result_t gyro_res;
  lsm303dlhc_acc_result_t acc_res;
  lsm303dlhc_mag_result_t mag_res;

  if (! (bmp085_run (sensors->bmp085, &baro_res, err) &&
         l3gd20_run (sensors->l3gd20, &gyro_res, err) &&
         lsm303dlhc_acc_run (sensors->lsm303dlhc_acc, &acc_res, err) &&
         lsm303dlhc_mag_run (sensors->lsm303dlhc_mag, &mag_res, err)))
    goto error;

  if (baro_res.have_result)
    fprintf ( stderr, "t=%.1f p=%.0f\n"
            , baro_res.temperature, baro_res.pressure );

  if (gyro_res.have_result)
    fprintf ( stderr, "gx=%.2f gy=%.2f gz=%.2f\n"
            , gyro_res.x, gyro_res.y, gyro_res.z );

  if (acc_res.have_result)
    fprintf ( stderr, "ax=%.2f ay=%.2f az=%.2f\n"
            , acc_res.x, acc_res.y, acc_res.z );

  if (mag_res.have_result)
    fprintf ( stderr, "mx=%.2f my=%.2f mz=%.2f\n"
            , mag_res.x, mag_res.y, mag_res.z );

  return true;

error:
  error_prefix (err, "i2c_sensors_run");
  return false;
}
