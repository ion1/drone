#include <error.h>
#include <stdio.h>

#include "error-utilities.h"
#include "i2c-sensors.h"

int
main (int argc, char **argv)
{
  ERROR_DECLARE (err);

  i2c_sensors_t *sensors = i2c_sensors_new ("/dev/i2c-1", 38, &err);
  if (! sensors)
    goto error;

  i2c_sensors_dump (sensors, stderr);

  for (int n = 0; n < 10000; ++n)
    if (! i2c_sensors_run (sensors, &err))
      goto error;

  i2c_sensors_free (sensors);

  return 0;

error:
  fprintf (stderr, "%s: %s\n", argv[0], err.message);
  return 1;
}
