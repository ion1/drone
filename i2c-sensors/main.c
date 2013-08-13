#include <error.h>
#include <stdio.h>

#include "i2c-sensors.h"

int
main (void)
{
  char *errstr = "Unknown error";
  int err = 0;

  i2c_sensors_t *sensors = i2c_sensors_new ("/dev/i2c-1", 38, &errstr, &err);
  if (! sensors)
    error_at_line (1, err, __FILE__, __LINE__, errstr);

  i2c_sensors_dump (sensors, stderr);

  for (int n = 0; n < 10000; ++n)
    i2c_sensors_run (sensors);

  i2c_sensors_free (sensors);

  return 0;
}
