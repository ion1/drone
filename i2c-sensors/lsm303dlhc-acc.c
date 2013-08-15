/* LSM303DLHC accelerometer */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "lsm303dlhc-acc.h"

#include "common.h"
#include "i2c-utilities.h"

#define ADDR 0x19

#define CTRL_REG1 0x20
#define CTRL_REG2 0x21
#define CTRL_REG3 0x22
#define CTRL_REG4 0x23
#define CTRL_REG5 0x24
#define CTRL_REG6 0x25
#define FIFO_CTRL_REG 0x2e

#define STATUS_REG 0x27
#define OUT_X_L 0x28
#define OUT_X_H 0x29
#define OUT_Y_L 0x2a
#define OUT_Y_H 0x2b
#define OUT_Z_L 0x2c
#define OUT_Z_H 0x2d

#define CTRL_REG1_ODR3 (1<<7)
#define CTRL_REG1_ODR2 (1<<6)
#define CTRL_REG1_ODR1 (1<<5)
#define CTRL_REG1_ODR0 (1<<4)
#define CTRL_REG1_LPen (1<<3)
#define CTRL_REG1_Zen  (1<<2)
#define CTRL_REG1_Yen  (1<<1)
#define CTRL_REG1_Xen  (1<<0)

#define CTRL_REG2_HPM1    (1<<7)
#define CTRL_REG2_HPM0    (1<<6)
#define CTRL_REG2_HPCF2   (1<<5)
#define CTRL_REG2_HPCF1   (1<<4)
#define CTRL_REG2_FDS     (1<<3)
#define CTRL_REG2_HPCLICK (1<<2)
#define CTRL_REG2_HPIS2   (1<<1)
#define CTRL_REG2_HPIS1   (1<<0)

#define CTRL_REG3_I1_CLICK   (1<<7)
#define CTRL_REG3_I1_AOI1    (1<<6)
#define CTRL_REG3_I1_AOI2    (1<<5)
#define CTRL_REG3_I1_DRDY1   (1<<4)
#define CTRL_REG3_I1_DRDY2   (1<<3)
#define CTRL_REG3_I1_WTM     (1<<2)
#define CTRL_REG3_I1_OVERRUN (1<<1)

#define CTRL_REG4_BDU (1<<7)
#define CTRL_REG4_BLE (1<<6)
#define CTRL_REG4_FS1 (1<<5)
#define CTRL_REG4_FS0 (1<<4)
#define CTRL_REG4_HR  (1<<3)
#define CTRL_REG4_SIM (1<<0)

#define CTRL_REG5_BOOT     (1<<7)
#define CTRL_REG5_FIFO_EN  (1<<6)
#define CTRL_REG5_LIR_INT1 (1<<3)
#define CTRL_REG5_D4D_INT1 (1<<2)
#define CTRL_REG5_LIR_INT2 (1<<1)
#define CTRL_REG5_D4D_INT2 (1<<0)

#define CTRL_REG6_I2_CLICKen (1<<7)
#define CTRL_REG6_I2_INT1    (1<<6)
#define CTRL_REG6_I2_INT2    (1<<5)
#define CTRL_REG6_BOOT_I1    (1<<4)
#define CTRL_REG6_P2_ACT     (1<<3)
#define CTRL_REG6_H_LACTIVE  (1<<1)

#define FIFO_CTRL_REG_FM1  (1<<7)
#define FIFO_CTRL_REG_FM0  (1<<6)
#define FIFO_CTRL_REG_TR   (1<<5)
#define FIFO_CTRL_REG_FTH4 (1<<4)
#define FIFO_CTRL_REG_FTH3 (1<<3)
#define FIFO_CTRL_REG_FTH2 (1<<2)
#define FIFO_CTRL_REG_FTH1 (1<<1)
#define FIFO_CTRL_REG_FTH0 (1<<0)

#define STATUS_REG_ZYXOR (1<<7)
#define STATUS_REG_ZOR   (1<<6)
#define STATUS_REG_YOR   (1<<5)
#define STATUS_REG_XOR   (1<<4)
#define STATUS_REG_ZYXDA (1<<3)
#define STATUS_REG_ZDA   (1<<2)
#define STATUS_REG_YDA   (1<<1)
#define STATUS_REG_XDA   (1<<0)

struct lsm303dlhc_acc {
  int fd;
};

lsm303dlhc_acc_t *
lsm303dlhc_acc_new (const int i2c_fd, error_t *const err)
{
  lsm303dlhc_acc_t *acc = malloc (sizeof (lsm303dlhc_acc_t));
  if (! acc) {
    error_errno (err);
    error_prefix (err, "malloc failed");
    goto malloc_failed;
  }

  acc->fd = i2c_fd;

  if (! i2c_slave (i2c_fd, ADDR, err))
    goto i2c_slave_failed;

  uint8_t reg1 = CTRL_REG1_ODR3 | CTRL_REG1_ODR0
               | CTRL_REG1_Zen | CTRL_REG1_Yen | CTRL_REG1_Xen
        , reg4 = CTRL_REG4_BDU | CTRL_REG4_BLE | CTRL_REG4_FS1 | CTRL_REG4_FS0
               | CTRL_REG4_HR;
  if (! (i2c_write_u8 (i2c_fd, CTRL_REG1, reg1, err) &&
         i2c_write_u8 (i2c_fd, CTRL_REG2, 0,    err) &&
         i2c_write_u8 (i2c_fd, CTRL_REG3, 0,    err) &&
         i2c_write_u8 (i2c_fd, CTRL_REG4, reg4, err) &&
         i2c_write_u8 (i2c_fd, CTRL_REG5, 0,    err) &&
         i2c_write_u8 (i2c_fd, CTRL_REG6, 0,    err) &&
         i2c_write_u8 (i2c_fd, FIFO_CTRL_REG, 0, err))) {
    error_prefix (err, "initialization");
    goto init_failed;
  }

  return acc;

init_failed:
i2c_slave_failed:
  free (acc);

malloc_failed:
  error_prefix (err, "lsm303dlhc_acc_new");
  return NULL;
}

void
lsm303dlhc_acc_free (lsm303dlhc_acc_t *const acc)
{
  acc->fd = POISON;
  free (acc);
}

bool
lsm303dlhc_acc_run ( lsm303dlhc_acc_t *const acc
                   , lsm303dlhc_acc_result_t *const res, error_t *const err )
{
  uint8_t  status;
  uint16_t x, y, z;

  res->have_result = false;

  if (! i2c_slave (acc->fd, ADDR, err))
    goto error;

  if (! i2c_read_u8 (acc->fd, STATUS_REG, &status, err))
    goto error;

  /* No new data available? */
  if (! (status & STATUS_REG_ZYXDA))
    return true;

  /* New data available. */
  if (! (i2c_read_u16 (acc->fd, OUT_X_L, &x, err) &&
         i2c_read_u16 (acc->fd, OUT_Y_L, &y, err) &&
         i2c_read_u16 (acc->fd, OUT_Z_L, &z, err)))
    goto error;

  /* 12: FS1|FS0
   * 1<<4: The chip pads values with four zero LSBs
   * 9.80665/1000: mg to m/s²
   */
  double scale = 9.80665 * 12.0 / ((double)(1<<4) * 1000.0);
  res->have_result = true;
  res->x = scale * (int16_t)x;
  res->y = scale * (int16_t)y;
  res->z = scale * (int16_t)z;

  return true;

error:
  error_prefix (err, "lsm303dlhc_acc_run");
  return false;
}
