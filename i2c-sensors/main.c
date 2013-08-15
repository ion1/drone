#include <error.h>
#include <math.h>
#include <stdio.h>

#include "error-utilities.h"
#include "i2c-sensors.h"

/* Air pressure at sea level in Pa.
 * http://weather.noaa.gov/pub/data/observations/metar/decoded/EFTP.TXT
 * http://www.aviationweather.gov/adds/metars/?station_ids=EFTP&std_trans=standard&chk_metars=on&hoursStr=past+36+hours&chk_tafs=on&submitmet=Submit
 */
#define P_SEA 100500

static void
print_res (const i2c_sensors_result_t *const res);

static inline double
magnitude (const double x, const double y, const double z);

int
main (int argc, char **argv)
{
  ERROR_DECLARE (err);

  i2c_sensors_t *sensors = i2c_sensors_new ("/dev/i2c-1", 38, &err);
  if (! sensors)
    goto error;

  i2c_sensors_dump (sensors, stderr);

  printf ("   °C    kPa    m | °/s  (x)  (y)  (z) | "
          " m/s²    (x)    (y)    (z) |   µT   (x)   (y)   (z)\n");

  i2c_sensors_result_t res;
  for (int n = 0; n < 1000; ++n) {
    if (! i2c_sensors_run (sensors, &res, &err))
      goto error;
    print_res (&res);
  }

  i2c_sensors_free (sensors);

  return 0;

error:
  fprintf (stderr, "%s: %s\n", argv[0], err.message);
  return 1;
}

static void
print_res (const i2c_sensors_result_t *const res)
{
  if (res->baro.have_result) {
    double alt = 44330.0 * (1.0 - pow (res->baro.pressure/P_SEA, 1.0/5.255));
    printf ( "% 5.1f %6.2f % 4.0f | "
           , res->baro.temperature, res->baro.pressure/1000.0, alt);
  } else {
    printf ("%5s %6s %4s | ", "", "", "");
  }

  if (res->gyro.have_result) {
    double c = 180.0/M_PI;
    printf ( "%3.0f % 4.0f % 4.0f % 4.0f | "
           , magnitude (res->gyro.x, res->gyro.y, res->gyro.z)*c
           , res->gyro.x*c, res->gyro.y*c, res->gyro.z*c );
  } else {
    printf ("%3s %4s %4s %4s | ", "", "", "", "");
  }

  if (res->acc.have_result) {
    printf ( "%5.2f % 6.2f % 6.2f % 6.2f | "
           , magnitude (res->acc.x, res->acc.y, res->acc.z)
           , res->acc.x, res->acc.y, res->acc.z );
  } else {
    printf ("%5s %6s %6s %6s | ", "", "", "", "");
  }

  if (res->mag.have_result) {
    printf ("%4.1f % 5.1f % 5.1f % 5.1f\n"
           , magnitude (res->mag.x, res->mag.y, res->mag.z)*1000000
           , res->mag.x*1000000, res->mag.y*1000000, res->mag.z*1000000);
  } else {
    printf ("%4s %5s %5s %5s\n", "", "", "", "");
  }

  fflush (stdout);
}

static inline double
magnitude (const double x, const double y, const double z)
{
  return sqrt (x*x + y*y + z*z);
}
