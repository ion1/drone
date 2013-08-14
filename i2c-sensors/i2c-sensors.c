#include <errno.h>
#include <fcntl.h>
#include <math.h>
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

/* Air pressure at sea level in Pa.
 * http://weather.noaa.gov/pub/data/observations/metar/decoded/EFTP.TXT
 * http://www.aviationweather.gov/adds/metars/?station_ids=EFTP&std_trans=standard&chk_metars=on&hoursStr=past+36+hours&chk_tafs=on&submitmet=Submit
 */
#define P_SEA 100500

struct i2c_sensors {
  int fd;
  bmp085_t *bmp085;
  l3gd20_t *l3gd20;
  lsm303dlhc_acc_t *lsm303dlhc_acc;
  lsm303dlhc_mag_t *lsm303dlhc_mag;
};

static inline double
mag (const double x, const double y, const double z);

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

  printf ("   °C    kPa    m | °/s  (x)  (y)  (z) | "
          " m/s²    (x)    (y)    (z) |   µT   (x)   (y)   (z)\n");

  if (baro_res.have_result) {
    double alt = 44330.0 * (1.0 - pow (baro_res.pressure/P_SEA, 1.0/5.255));
    printf ( "% 5.1f %6.2f % 4.0f | "
           , baro_res.temperature, baro_res.pressure/1000.0, alt);
  } else {
    printf ("%5s %6s %4s | ", "", "", "");
  }

  if (gyro_res.have_result) {
    double c = 180.0/M_PI;
    printf ( "%3.0f % 4.0f % 4.0f % 4.0f | "
           , mag (gyro_res.x, gyro_res.y, gyro_res.z)*c
           , gyro_res.x*c, gyro_res.y*c, gyro_res.z*c );
  } else {
    printf ("%3s %4s %4s %4s | ", "", "", "", "");
  }

  if (acc_res.have_result) {
    printf ( "%5.2f % 6.2f % 6.2f % 6.2f | "
           , mag (acc_res.x, acc_res.y, acc_res.z)
           , acc_res.x, acc_res.y, acc_res.z );
  } else {
    printf ("%5s %6s %6s %6s | ", "", "", "", "");
  }

  if (mag_res.have_result) {
    printf ("%4.1f % 5.1f % 5.1f % 5.1f\n"
           , mag (mag_res.x, mag_res.y, mag_res.z)*1000000
           , mag_res.x*1000000, mag_res.y*1000000, mag_res.z*1000000);
  } else {
    printf ("%4s %5s %5s %5s\n", "", "", "", "");
  }

  fflush (stdout);

  return true;

error:
  error_prefix (err, "i2c_sensors_run");
  return false;
}

static inline double
mag (const double x, const double y, const double z)
{
  return sqrt (x*x + y*y + z*z);
}
