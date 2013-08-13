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
#include "l3gd20.h"

struct i2c_sensors {
  int fd;
  bmp085_t *bmp085;
  l3gd20_t *l3gd20;
};

i2c_sensors_t *
i2c_sensors_new ( const char *const dev, const int bmp085_eoc_gpio
                , char **errstr, int *err )
{
  i2c_sensors_t *sensors = malloc (sizeof (i2c_sensors_t));
  if (! sensors) {
    *errstr = "i2c_sensors_new: malloc failed";
    *err = errno;
    goto malloc_failed;
  }

  if ((sensors->fd = open (dev, O_RDWR)) < 0) {
    *errstr = "i2c_sensors_new: opening i2c device failed";
    *err = errno;
    goto open_failed;
  }

  if (! (sensors->bmp085 = bmp085_new ( sensors->fd, bmp085_eoc_gpio, 3
                                      , errstr, err )))
    goto bmp085_failed;

  if (! (sensors->l3gd20 = l3gd20_new (sensors->fd, errstr, err)))
    goto l3gd20_failed;

  return sensors;

/* everything_failed: */
  l3gd20_free (sensors->l3gd20);

l3gd20_failed:
  bmp085_free (sensors->bmp085);

bmp085_failed:
  close (sensors->fd);

open_failed:
  free (sensors);

malloc_failed:
  return NULL;
}

void
i2c_sensors_free (i2c_sensors_t *const sensors)
{
  bmp085_free (sensors->bmp085);
  sensors->bmp085 = (bmp085_t *)POISON;

  l3gd20_free (sensors->l3gd20);
  sensors->l3gd20 = (l3gd20_t *)POISON;

  close (sensors->fd);
  sensors->fd = POISON;

  free (sensors);
}

void
i2c_sensors_dump (const i2c_sensors_t *const sensors, FILE *const stream)
{
  bmp085_dump (sensors->bmp085, stream);
}

void
i2c_sensors_run (i2c_sensors_t *const sensors)
{
  int32_t t = 0, p = 0;
  if (bmp085_run (sensors->bmp085, &t, &p))
    fprintf (stderr, "t=%d p=%d\n", t, p);

  double gx = 0, gy = 0, gz = 0;
  if (l3gd20_run (sensors->l3gd20, &gx, &gy, &gz))
    fprintf (stderr, "gx=%.2f gy=%.2f gz=%.2f\n", gx, gy, gz);
}
