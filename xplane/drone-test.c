#define __STDC_FORMAT_MACROS

#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <XPLMDataAccess.h>
#include <XPLMPlugin.h>
#include <XPLMProcessing.h>
#include <XPLMUtilities.h>

static void
xplm_vprintf (const char *const format, va_list ap)
{
  char buf[1000] = "";
  (void)vsnprintf (buf, 1000, format, ap);
  XPLMDebugString (buf);
}

#if 0
static void
xplm_printf (const char *const format, ...)
  __attribute__ ((format (printf, 1, 2)));

static void
xplm_printf (const char *const format, ...)
{
  va_list ap;
  va_start (ap, format);
  xplm_vprintf (format, ap);
  va_end (ap);
}
#endif

static void
xplm_log (const char *const format, ...)
  __attribute__ ((format (printf, 1, 2)));

static void
xplm_log (const char *const format, ...)
{
  char format_buf[1000] = "";
  (void)snprintf (format_buf, 1000, "DroneTest: %s\n", format);

  va_list ap;
  va_start (ap, format);
  xplm_vprintf (format_buf, ap);
  va_end (ap);
}

#define LAT_R "sim/flightmodel/position/latitude"   /* double ° */
#define LON_R "sim/flightmodel/position/longitude"  /* double ° */
#define ELE_R "sim/flightmodel/position/elevation"  /* double m */

#define HEADING_TRUE_R "sim/flightmodel/position/psi"     /* float ° */
#define HEADING_MAG_R  "sim/flightmodel/position/magpsi"  /* float ° */

#define ACC_X_R "sim/flightmodel/position/local_ax"  /* float m/s² */
#define ACC_Y_R "sim/flightmodel/position/local_ay"  /* float m/s² */
#define ACC_Z_R "sim/flightmodel/position/local_az"  /* float m/s² */
#define GRAV_R  "sim/weather/gravity_mss"            /* float m/s² */

#define ROLL_RATE_R  "sim/flightmodel/position/Prad"  /* float rad/s */
#define PITCH_RATE_R "sim/flightmodel/position/Qrad"  /* float rad/s */
#define YAW_RATE_R   "sim/flightmodel/position/Rrad"  /* float rad/s */

#define PRES_SEA_R "sim/weather/barometer_sealevel_inhg"  /* float inHg */
#define PRES_R     "sim/weather/barometer_current_inhg"   /* float inHg */
#define TEMP_R     "sim/weather/temperature_ambient_c"    /* float °C */

static XPLMDataRef latitude_r, longitude_r, elevation_r, heading_r
                 , ax_r, ay_r, az_r, gravity_r
                 , roll_rate_r, pitch_rate_r, yaw_rate_r
                 , pressure_sea_r, pressure_r, temperature_r;

static XPLMFlightLoopID flight_loop_id;

static inline double
inHg_to_Pa (const double inHg)
{
  /* 1 inHg = 25.4 mmHg */
  /* 760 mmHg = 101325 Pa */
  return inHg * 25.4 * 101325.0 / 760.0;
}

static float
flight_loop_cb ( float elapsed_since_last_call
               , float elapsed_since_last_flight_loop
               , int counter
               , void *refcon )
{
  (void)elapsed_since_last_call;
  (void)elapsed_since_last_flight_loop;
  (void)counter;
  (void)refcon;

  double lat = XPLMGetDatad (latitude_r)
       , lon = XPLMGetDatad (longitude_r)
       , ele = XPLMGetDatad (elevation_r)
       , heading = XPLMGetDataf (heading_r)
       , ax = XPLMGetDataf (ax_r)
       , ay = XPLMGetDataf (ay_r)
       , az = XPLMGetDataf (az_r)
       , grav = XPLMGetDataf (gravity_r)
       , roll_rate = XPLMGetDataf (roll_rate_r)
       , pitch_rate = XPLMGetDataf (pitch_rate_r)
       , yaw_rate = XPLMGetDataf (yaw_rate_r)
       , pres_sea = inHg_to_Pa (XPLMGetDataf (pressure_sea_r))
       , pres = inHg_to_Pa (XPLMGetDataf (pressure_r))
       , temp = XPLMGetDataf (temperature_r)
       ;

  double pres_alt = 44330.0 * (1.0 - pow (pres/pres_sea, 1.0/5.255));

  xplm_log ("BEGIN");
  xplm_log ("lat=%.6f lon=%.6f ele=%.1f", lat, lon, ele);
  xplm_log ("heading=%.1f", heading);
  xplm_log ("ax=%.2f ay=%.2f az=%.2f grav=%.2f", ax, ay, az, grav);
  xplm_log ("rollr=%.0f pitchr=%.0f yawr=%.0f", roll_rate/M_PI*180.0, pitch_rate/M_PI*180.0, yaw_rate/M_PI*180.0);
  xplm_log ("pres_sea=%.2f pres=%.2f temp=%.1f (alt=%.1f)", pres_sea/1000.0, pres/1000.0, temp, pres_alt);
  xplm_log ("END");

  return 1.0;
}

PLUGIN_API int
XPluginStart (char *out_name, char *out_sig, char *out_desc)
{
  strcpy (out_name, "Drone Test");
  strcpy (out_sig,  "fi.heh.drone.test");
  strcpy (out_desc, "Testing and stuff");

  xplm_log ("XPluginStart");

  if (! ((latitude_r     = XPLMFindDataRef (LAT_R)) &&
         (longitude_r    = XPLMFindDataRef (LON_R)) &&
         (elevation_r    = XPLMFindDataRef (ELE_R)) &&
         (heading_r      = XPLMFindDataRef (HEADING_MAG_R)) &&
         (ax_r           = XPLMFindDataRef (ACC_X_R)) &&
         (ay_r           = XPLMFindDataRef (ACC_Y_R)) &&
         (az_r           = XPLMFindDataRef (ACC_Z_R)) &&
         (gravity_r      = XPLMFindDataRef (GRAV_R)) &&
         (roll_rate_r    = XPLMFindDataRef (ROLL_RATE_R)) &&
         (pitch_rate_r   = XPLMFindDataRef (PITCH_RATE_R)) &&
         (yaw_rate_r     = XPLMFindDataRef (YAW_RATE_R)) &&
         (pressure_sea_r = XPLMFindDataRef (PRES_SEA_R)) &&
         (pressure_r     = XPLMFindDataRef (PRES_R)) &&
         (temperature_r  = XPLMFindDataRef (TEMP_R)))) {
    xplm_log ("Failed to find all data refs");
    return 0;
  }

  XPLMCreateFlightLoop_t fl = { .structSize = sizeof (XPLMCreateFlightLoop_t)
                              , .phase = xplm_FlightLoop_Phase_AfterFlightModel
                              , .callbackFunc = flight_loop_cb
                              , .refcon = NULL
                              };
  flight_loop_id = XPLMCreateFlightLoop (&fl);

  return 1;
}

PLUGIN_API void
XPluginStop (void)
{
  xplm_log ("XPluginStop");

  XPLMDestroyFlightLoop (flight_loop_id);
}

PLUGIN_API void
XPluginDisable (void)
{
  xplm_log ("XPluginDisable");
}

PLUGIN_API int
XPluginEnable (void)
{
  xplm_log ("XPluginEnable");

  XPLMScheduleFlightLoop (flight_loop_id, 1.0, 1);
  return 1;
}

PLUGIN_API void
XPluginReceiveMessage (XPLMPluginID sender, int message, void *param)
{
  xplm_log ("Received message %x %" PRIxPTR, message, (intptr_t)param);
}
