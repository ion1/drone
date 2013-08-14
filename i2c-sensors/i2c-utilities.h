#ifndef INCLUDE_I2C_UTILITIES_H
#define INCLUDE_I2C_UTILITIES_H

/* i2c-dev.h uses NULL but doesn’t include stddef.h */
#include <stddef.h>

#include <errno.h>
#include <linux/i2c-dev.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/ioctl.h>

#include "error-utilities.h"

static inline bool
i2c_slave (const int fd, const int addr, error_t *const err) {
  if (ioctl (fd, I2C_SLAVE, addr) < 0) {
    error_strerror (err, errno);
    error_prefix_printf (err, "i2c_slave: ioctl I2C_SLAVE %d failed", addr);
    return false;
  }

  return true;
}

static inline bool
i2c_read_u8 ( const int fd, const uint8_t command, uint8_t *const data
            , error_t *const err )
{
  int32_t b0;
  if ((b0 = i2c_smbus_read_byte_data (fd, command)) < 0) {
    error_strerror (err, errno);
    error_prefix (err, "i2c_read_u8: i2c_smbus_read_byte_data failed");
    return false;
  }

  *data = b0;
  return true;
}

static inline bool
i2c_read_u16 ( const int fd, const uint8_t command, uint16_t *const data
             , error_t *const err )
{
  int32_t b0, b1;
  if (((b0 = i2c_smbus_read_byte_data (fd, command+0)) < 0) ||
      ((b1 = i2c_smbus_read_byte_data (fd, command+1)) < 0)) {
    error_strerror (err, errno);
    error_prefix (err, "i2c_read_u16: i2c_smbus_read_byte_data failed");
    return false;
  }

  *data = (b0<<8) | b1;
  return true;
}

static inline bool
i2c_read_u24 ( const int fd, const uint8_t command, uint32_t *const data
             , error_t *const err )
{
  int32_t b0, b1, b2;
  if (((b0 = i2c_smbus_read_byte_data (fd, command+0)) < 0) ||
      ((b1 = i2c_smbus_read_byte_data (fd, command+1)) < 0) ||
      ((b2 = i2c_smbus_read_byte_data (fd, command+2)) < 0)) {
    error_strerror (err, errno);
    error_prefix (err, "i2c_read_u24: i2c_smbus_read_byte_data failed");
    return false;
  }

  *data = (b0<<16) | (b1<<8) | b2;
  return true;
}

static inline bool
i2c_read_u32 ( const int fd, const uint8_t command, uint32_t *const data
             , error_t *const err )
{
  int32_t b0, b1, b2, b3;
  if (((b0 = i2c_smbus_read_byte_data (fd, command+0)) < 0) ||
      ((b1 = i2c_smbus_read_byte_data (fd, command+1)) < 0) ||
      ((b2 = i2c_smbus_read_byte_data (fd, command+2)) < 0) ||
      ((b3 = i2c_smbus_read_byte_data (fd, command+3)) < 0)) {
    error_strerror (err, errno);
    error_prefix (err, "i2c_read_u32: i2c_smbus_read_byte_data failed");
    return false;
  }

  *data = (b0<<24) | (b1<<16) | (b2<<8) | b3;
  return true;
}

static inline bool
i2c_write_u8 ( const int fd, const uint8_t command, const uint8_t data
             , error_t *const err )
{
  if (i2c_smbus_write_byte_data (fd, command, data) < 0) {
    error_strerror (err, errno);
    error_prefix (err, "i2c_write_u8: i2c_smbus_write_byte_data failed");
    return false;
  }

  return true;
}

#endif /* INCLUDE_I2C_UTILITIES_H */
