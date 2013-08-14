/* The same as HMC5883L? */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "lsm303dlhc-mag.h"

#include "common.h"
#include "i2c-utilities.h"

#define ADDR 0x1e

#define CRA_REG 0x00
#define CRB_REG 0x01
#define MR_REG  0x02
#define OUT_X_H 0x03
#define OUT_X_L 0x04
#define OUT_Z_H 0x05
#define OUT_Z_L 0x06
#define OUT_Y_H 0x07
#define OUT_Y_L 0x08
#define SR_REG  0x09
#define TEMP_OUT_H 0x31
#define TEMP_OUT_L 0x32

#define CRA_REG_TEMP_EN (1<<7)
#define CRA_REG_DO2     (1<<4)
#define CRA_REG_DO1     (1<<3)
#define CRA_REG_DO0     (1<<2)

#define CRB_REG_GN2 (1<<7)
#define CRB_REG_GN1 (1<<6)
#define CRB_REG_GN0 (1<<5)

#define MR_REG_MD1 (1<<1)
#define MR_REG_MD0 (1<<0)

#define SR_REG_LOCK (1<<1)
#define SR_REG_DRDY (1<<0)

struct lsm303dlhc_mag {
  int fd;
};

lsm303dlhc_mag_t *
lsm303dlhc_mag_new (const int i2c_fd, error_t *const err)
{
  lsm303dlhc_mag_t *mag = malloc (sizeof (lsm303dlhc_mag_t));
  if (! mag) {
    error_strerror (err, errno);
    error_prefix (err, "malloc failed");
    goto malloc_failed;
  }

  mag->fd = i2c_fd;

  if (! i2c_slave (i2c_fd, ADDR, err))
    goto i2c_slave_failed;

  uint8_t cra = CRA_REG_DO2 | CRA_REG_DO1 | CRA_REG_DO0
        , crb = CRB_REG_GN2 | CRB_REG_GN1 | CRB_REG_GN0;
  if (! (i2c_write_u8 (i2c_fd, CRA_REG, cra, err) &&
         i2c_write_u8 (i2c_fd, CRB_REG, crb, err) &&
         i2c_write_u8 (i2c_fd, MR_REG,  0,   err))) {
    error_prefix (err, "initialization");
    goto init_failed;
  }

  return mag;

init_failed:
i2c_slave_failed:
  free (mag);

malloc_failed:
  error_prefix (err, "lsm303dlhc_mag_new");
  return NULL;
}

void
lsm303dlhc_mag_free (lsm303dlhc_mag_t *const mag)
{
  mag->fd = POISON;
  free (mag);
}

bool
lsm303dlhc_mag_run ( lsm303dlhc_mag_t *const mag
                   , lsm303dlhc_mag_result_t *const res, error_t *const err)
{
  uint8_t  status;
  uint16_t x, y, z;

  res->have_result = false;

  if (! i2c_slave (mag->fd, ADDR, err))
    goto error;

  if (! i2c_read_u8 (mag->fd, SR_REG, &status, err))
    goto error;

  /* No new data available? */
  if (! (status & SR_REG_DRDY))
    return true;

  /* New data available. */
  if (! (i2c_read_u16 (mag->fd, OUT_X_H, &x, err) &&
         i2c_read_u16 (mag->fd, OUT_Z_H, &z, err) &&
         i2c_read_u16 (mag->fd, OUT_Y_H, &y, err)))
    goto error;

  /* 230, 205: GN2|GN1|GN0
   */
  double scalexy = 1.0/230.0
       , scalez  = 1.0/205.0;
  res->have_result = true;
  res->x = scalexy * (int16_t)x;
  res->y = scalexy * (int16_t)y;
  res->z = scalez  * (int16_t)z;

  return true;

error:
  error_prefix (err, "lsm303dlhc_mag_run");
  return false;
}
