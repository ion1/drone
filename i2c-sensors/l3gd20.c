#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

#include "l3gd20.h"

#include "common.h"
#include "i2c-utilities.h"

#define L3GD20_ADDR 0x6b

#define L3GD20_CTRL_REG1 0x20
#define L3GD20_CTRL_REG2 0x21
#define L3GD20_CTRL_REG3 0x22
#define L3GD20_CTRL_REG4 0x23
#define L3GD20_CTRL_REG5 0x24
#define L3GD20_FIFO_CTRL_REG 0x2e

#define L3GD20_OUT_TEMP 0x26
#define L3GD20_STATUS_REG 0x27
#define L3GD20_OUT_X_L 0x28
#define L3GD20_OUT_X_H 0x29
#define L3GD20_OUT_Y_L 0x2a
#define L3GD20_OUT_Y_H 0x2b
#define L3GD20_OUT_Z_L 0x2c
#define L3GD20_OUT_Z_H 0x2d

#define L3GD20_CTRL_REG1_DR1 (1<<7)
#define L3GD20_CTRL_REG1_DR0 (1<<6)
#define L3GD20_CTRL_REG1_BW1 (1<<5)
#define L3GD20_CTRL_REG1_BW0 (1<<4)
#define L3GD20_CTRL_REG1_PD  (1<<3)
#define L3GD20_CTRL_REG1_Zen (1<<2)
#define L3GD20_CTRL_REG1_Yen (1<<1)
#define L3GD20_CTRL_REG1_Xen (1<<0)

#define L3GD20_CTRL_REG2_HPM1  (1<<5)
#define L3GD20_CTRL_REG2_HPM0  (1<<4)
#define L3GD20_CTRL_REG2_HPCF3 (1<<3)
#define L3GD20_CTRL_REG2_HPCF2 (1<<2)
#define L3GD20_CTRL_REG2_HPCF1 (1<<1)
#define L3GD20_CTRL_REG2_HPCF0 (1<<0)

#define L3GD20_CTRL_REG3_I1_Int1   (1<<7)
#define L3GD20_CTRL_REG3_I1_Boot   (1<<6)
#define L3GD20_CTRL_REG3_H_Lactive (1<<5)
#define L3GD20_CTRL_REG3_PP_OD     (1<<4)
#define L3GD20_CTRL_REG3_I2_DRDY   (1<<3)
#define L3GD20_CTRL_REG3_I2_WTM    (1<<2)
#define L3GD20_CTRL_REG3_I2_ORun   (1<<1)
#define L3GD20_CTRL_REG3_I2_Empty  (1<<0)

#define L3GD20_CTRL_REG4_BDU (1<<7)
#define L3GD20_CTRL_REG4_BLE (1<<6)
#define L3GD20_CTRL_REG4_FS1 (1<<5)
#define L3GD20_CTRL_REG4_FS0 (1<<4)
#define L3GD20_CTRL_REG4_SIM (1<<0)

#define L3GD20_CTRL_REG5_BOOT      (1<<7)
#define L3GD20_CTRL_REG5_FIFO_EN   (1<<6)
#define L3GD20_CTRL_REG5_HPen      (1<<4)
#define L3GD20_CTRL_REG5_INT1_Sel1 (1<<3)
#define L3GD20_CTRL_REG5_INT1_Sel0 (1<<2)
#define L3GD20_CTRL_REG5_Out_Sel1  (1<<1)
#define L3GD20_CTRL_REG5_Out_Sel0  (1<<0)

#define L3GD20_FIFO_CTRL_REG_FM2  (1<<7)
#define L3GD20_FIFO_CTRL_REG_FM1  (1<<6)
#define L3GD20_FIFO_CTRL_REG_FM0  (1<<5)
#define L3GD20_FIFO_CTRL_REG_WTM4 (1<<4)
#define L3GD20_FIFO_CTRL_REG_WTM3 (1<<3)
#define L3GD20_FIFO_CTRL_REG_WTM2 (1<<2)
#define L3GD20_FIFO_CTRL_REG_WTM1 (1<<1)
#define L3GD20_FIFO_CTRL_REG_WTM0 (1<<0)

#define L3GD20_STATUS_REG_ZYXOR (1<<7)
#define L3GD20_STATUS_REG_ZOR   (1<<6)
#define L3GD20_STATUS_REG_YOR   (1<<5)
#define L3GD20_STATUS_REG_XOR   (1<<4)
#define L3GD20_STATUS_REG_ZYXDA (1<<3)
#define L3GD20_STATUS_REG_ZDA   (1<<2)
#define L3GD20_STATUS_REG_YDA   (1<<1)
#define L3GD20_STATUS_REG_XDA   (1<<0)

struct l3gd20 {
  int fd;
};

l3gd20_t *
l3gd20_new (const int i2c_fd, char **errstr, int *err)
{
  l3gd20_t *l3gd20 = malloc (sizeof (l3gd20_t));
  if (! l3gd20) {
    *errstr = "l3gd20_new: malloc failed";
    *err = errno;
    goto malloc_failed;
  }

  l3gd20->fd = i2c_fd;

  if (! i2c_slave (i2c_fd, L3GD20_ADDR)) {
    *errstr = "l3gd20_new: i2c_slave failed";
    *err = errno;
    goto i2c_slave_failed;
  }

  uint8_t reg1 = L3GD20_CTRL_REG1_DR1 | L3GD20_CTRL_REG1_DR0
               | L3GD20_CTRL_REG1_BW1 | L3GD20_CTRL_REG1_BW0
               | L3GD20_CTRL_REG1_PD  | L3GD20_CTRL_REG1_Zen
               | L3GD20_CTRL_REG1_Xen | L3GD20_CTRL_REG1_Yen
        , reg4 = L3GD20_CTRL_REG4_BDU | L3GD20_CTRL_REG4_BLE
               | L3GD20_CTRL_REG4_FS1 | L3GD20_CTRL_REG4_FS0;
  if (! (i2c_write_u8 (i2c_fd, L3GD20_CTRL_REG1, reg1) &&
         i2c_write_u8 (i2c_fd, L3GD20_CTRL_REG2, 0)    &&
         i2c_write_u8 (i2c_fd, L3GD20_CTRL_REG3, 0)    &&
         i2c_write_u8 (i2c_fd, L3GD20_CTRL_REG4, reg4) &&
         i2c_write_u8 (i2c_fd, L3GD20_CTRL_REG5, 0)    &&
         i2c_write_u8 (i2c_fd, L3GD20_FIFO_CTRL_REG, 0))) {
    *errstr = "l3gd20_new: i2c_write_u8 (initialization) failed";
    *err = errno;
    goto init_failed;
  }

  return l3gd20;

init_failed:
i2c_slave_failed:
  free (l3gd20);

malloc_failed:
  return NULL;
}

void
l3gd20_free (l3gd20_t *const l3gd20)
{
  l3gd20->fd = POISON;
  free (l3gd20);
}

bool
l3gd20_run ( l3gd20_t *const l3gd20, double *const x_out
           , double *const y_out, double *const z_out )
{
  uint8_t  status;
  uint16_t x, y, z;

  if (! i2c_slave (l3gd20->fd, L3GD20_ADDR))
    return false;

  if (! i2c_read_u8 (l3gd20->fd, L3GD20_STATUS_REG, &status))
    return false;

  if (! (status & L3GD20_STATUS_REG_ZYXDA))
    return false;

  /* New data available. */
  if (! (i2c_read_u16 (l3gd20->fd, L3GD20_OUT_X_L, &x) &&
         i2c_read_u16 (l3gd20->fd, L3GD20_OUT_Y_L, &y) &&
         i2c_read_u16 (l3gd20->fd, L3GD20_OUT_Z_L, &z))) {
    return false;
  }

  /* 70: FS1|FS0
   * 1000: m°/s to °/s
   */
  double scale = 70.0 / 1000.0;
  *x_out = scale * (int16_t)x;
  *y_out = scale * (int16_t)y;
  *z_out = scale * (int16_t)z;

  return true;
}
