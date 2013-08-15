/* BMP085 barometer */

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "bmp085.h"

#include "common.h"
#include "error-utilities.h"
#include "i2c-utilities.h"

#define ADDR 0x77

#define CALIB_REG 0xaa

#define CTRL_REG  0xf4
#define CTRL_TEMP 0x2e
#define CTRL_PRES 0x34

#define DATA 0xf6

typedef struct {
  int16_t  ac1, ac2, ac3;
  uint16_t ac4, ac5, ac6;
  int16_t  b1, b2, mb, mc, md;
} bmp085_calib_t;

#define CALIB_EXAMPLE \
  ((bmp085_calib_t){ 408, -72, -14383, 32741, 32757, 23153 \
                   , 6190, 4, -32768, -8711, 2868 \
                   })
#define UT_EXAMPLE  ((int32_t)27898)
#define OSS_EXAMPLE ((int16_t)0)
#define UP_EXAMPLE  ((int32_t)23843)

typedef enum { STATE_INITIAL, STATE_TEMP_WAITING, STATE_PRES_WAITING }
  bmp085_state_t;

struct bmp085 {
  int i2c_fd;
  int eoc_fd;
  int16_t oss;
  int32_t ut, up;
  bmp085_state_t state;
  bmp085_calib_t calib;
};

static bool slave (bmp085_t *const bmp085, error_t *const err);
static bool measure_temp_start (bmp085_t *const bmp085, error_t *const err);
static bool measure_pres_start (bmp085_t *const bmp085, error_t *const err);
static bool measure_temp_finish (bmp085_t *const bmp085, error_t *const err);
static bool measure_pres_finish (bmp085_t *const bmp085, error_t *const err);
static bool ready ( bmp085_t *const bmp085, bool *const is_ready
                  , error_t *const err );
static void calculate ( const bmp085_t *const bmp085
                      , bmp085_result_t *const res );

bmp085_t *
bmp085_new ( const int i2c_fd, const int eoc_gpio, const int16_t oss
           , error_t *const err )
{
  if (oss < 0 || oss > 3) {
    error_printf (err, "Invalid value for oss: %u", oss);
    goto invalid_oss;
  }

  bmp085_t *bmp085 = malloc (sizeof (bmp085_t));
  if (! bmp085) {
    error_strerror (err, errno);
    error_prefix (err, "malloc failed");
    goto malloc_failed;
  }

  bmp085->i2c_fd = i2c_fd;
  bmp085->state = STATE_INITIAL;
  bmp085->oss = oss;
  bmp085->ut = bmp085->up = 0;

  char eoc_gpio_path[100];
  if (snprintf (eoc_gpio_path, 100, GPIO_PATH, eoc_gpio) < 0) {
    error_insert (err, "snprintf failed");
    goto sprintf_failed;
  }

  if ((bmp085->eoc_fd = open (eoc_gpio_path, O_RDONLY)) < 0) {
    error_strerror (err, errno);
    error_prefix_printf (err, "open %s failed", eoc_gpio_path);
    goto open_gpio_failed;
  }

  if (! slave (bmp085, err))
    goto slave_failed;

  uint16_t calib_data[11];
  for (int i = 0; i < 11; ++i) {
    if (! i2c_read_u16 (i2c_fd, CALIB_REG+2*i, &calib_data[i], err)) {
      error_prefix (err, "calibration");
      goto calib_failed;
    }
  }
  bmp085->calib.ac1 = calib_data[0];
  bmp085->calib.ac2 = calib_data[1];
  bmp085->calib.ac3 = calib_data[2];
  bmp085->calib.ac4 = calib_data[3];
  bmp085->calib.ac5 = calib_data[4];
  bmp085->calib.ac6 = calib_data[5];
  bmp085->calib.b1  = calib_data[6];
  bmp085->calib.b2  = calib_data[7];
  bmp085->calib.mb  = calib_data[8];
  bmp085->calib.mc  = calib_data[9];
  bmp085->calib.md  = calib_data[10];

  return bmp085;

calib_failed:
slave_failed:
  close (bmp085->eoc_fd);

open_gpio_failed:
sprintf_failed:
  free (bmp085);

malloc_failed:
invalid_oss:
  error_prefix (err, "bmp085_new");
  return NULL;
}

void
bmp085_free (bmp085_t *const bmp085)
{
  close (bmp085->i2c_fd);
  bmp085->i2c_fd = POISON;

  close (bmp085->eoc_fd);
  bmp085->eoc_fd = POISON;

  bmp085->state = POISON;
  bmp085->calib.ac1 = bmp085->calib.ac2 = bmp085->calib.ac3 = (int16_t)POISON;
  bmp085->calib.ac4 = bmp085->calib.ac5 = bmp085->calib.ac6 = (uint16_t)POISON;
  bmp085->calib.b1  = bmp085->calib.b2  = (int16_t)POISON;
  bmp085->calib.mb  = bmp085->calib.mc  = bmp085->calib.md  = (int16_t)POISON;

  free (bmp085);
}

void
bmp085_dump (const bmp085_t *const bmp085, FILE *const stream)
{
  fprintf ( stream
          , "bmp085: i2c_fd=%d eoc_fd=%d state=%d "
            "ac1=%d ac2=%d ac3=%d ac4=%u ac5=%u ac6=%u "
            "b1=%d b2=%d mb=%d mc=%d md=%d\n"
          , bmp085->i2c_fd, bmp085->eoc_fd, bmp085->state
          , bmp085->calib.ac1, bmp085->calib.ac2, bmp085->calib.ac3
          , bmp085->calib.ac4, bmp085->calib.ac5, bmp085->calib.ac6
          , bmp085->calib.b1,  bmp085->calib.b2
          , bmp085->calib.mb,  bmp085->calib.mc,  bmp085->calib.md
          );
}

bool
bmp085_run ( bmp085_t *const bmp085, bmp085_result_t *const res
           , error_t *const err )
{
  res->have_result = false;

  if (bmp085->state == STATE_INITIAL) {
    if (! (slave (bmp085, err) &&
           measure_temp_start (bmp085, err)))
      goto error;

    bmp085->state = STATE_TEMP_WAITING;

  } else if (bmp085->state == STATE_TEMP_WAITING) {
    bool is_ready;
    if (! ready (bmp085, &is_ready, err))
      goto error;

    if (is_ready) {
      if (! (slave (bmp085, err) &&
             measure_temp_finish (bmp085, err) &&
             measure_pres_start (bmp085, err)))
        goto error;

      bmp085->state = STATE_PRES_WAITING;
    }

  } else if (bmp085->state == STATE_PRES_WAITING) {
    bool is_ready;
    if (! ready (bmp085, &is_ready, err)) 
      goto error;

    if (is_ready) {
      if (! (slave (bmp085, err) &&
             measure_pres_finish (bmp085, err) &&
             measure_temp_start (bmp085, err)))
        goto error;

      bmp085->state = STATE_TEMP_WAITING;

      calculate (bmp085, res);
    }
  }

  return true;

error:
  error_prefix (err, "bmp085_run");
  bmp085->state = STATE_INITIAL;
  return false;
}

static inline bool
slave (bmp085_t *const bmp085, error_t *const err)
{
  return i2c_slave (bmp085->i2c_fd, ADDR, err);
}

static bool
measure_temp_start (bmp085_t *const bmp085, error_t *const err)
{
  if (! i2c_write_u8 (bmp085->i2c_fd, CTRL_REG, CTRL_TEMP, err)) {
    error_prefix (err, "measure_temp_start");
    return false;
  }

  return true;
}

static bool
measure_pres_start (bmp085_t *const bmp085, error_t *const err)
{
  uint8_t data = CTRL_PRES + (bmp085->oss << 6);
  if (! i2c_write_u8 (bmp085->i2c_fd, CTRL_REG, data, err)) {
    error_prefix (err, "measure_pres_start");
    return false;
  }

  return true;
}

static bool
measure_temp_finish (bmp085_t *const bmp085, error_t *const err)
{
  uint16_t ut;
  if (! i2c_read_u16 (bmp085->i2c_fd, DATA, &ut, err)) {
    error_prefix (err, "measure_temp_finish");
    return false;
  }

  bmp085->ut = ut;
  return true;
}

static bool
measure_pres_finish (bmp085_t *const bmp085, error_t *const err)
{
  uint32_t up;
  if (! i2c_read_u24 (bmp085->i2c_fd, DATA, &up, err)) {
    error_prefix (err, "measure_pres_finish");
    return false;
  }

  bmp085->up = up >> (8 - bmp085->oss);
  return true;
}

static bool
ready (bmp085_t *const bmp085, bool *const is_ready, error_t *const err)
{
  char buf[100];
  ssize_t count;

  if (lseek (bmp085->eoc_fd, 0, SEEK_SET) == -1) {
    error_strerror (err, errno);
    error_prefix (err, "seek failed");
    goto error;
  }

  count = read (bmp085->eoc_fd, buf, 100);
  if (count == -1) {
    error_strerror (err, errno);
    error_prefix (err, "read failed");
    goto error;
  }

  *is_ready = count >= 2 && buf[0] == '1' && buf[1] == '\n';
  return true;

error:
  error_prefix (err, "ready");
  return false;
}

static void
calculate (const bmp085_t *const bmp085, bmp085_result_t *const res)
{
  int16_t oss = bmp085->oss;
  int32_t ut  = bmp085->ut
        , up  = bmp085->up;
  const bmp085_calib_t *calib = &bmp085->calib;

  int32_t x1a = ((ut - calib->ac6) * calib->ac5) >> 15;
  int32_t x2a = (calib->mc << 11) / (x1a + calib->md);
  int32_t b5  = x1a + x2a;
  int32_t t   = (b5 + 8) >> 4;

  int32_t b6  = b5 - 4000;
  int32_t x1b = (calib->b2 * ((b6 * b6) >> 12)) >> 11;
  int32_t x2b = (calib->ac2 * b6) >> 11;
  int32_t x3b = x1b + x2b;
  int32_t b3  = (((calib->ac1*4 + x3b) << oss) + 2) >> 2;
  int32_t x1c = (calib->ac3 * b6) >> 13;
  int32_t x2c = (calib->b1 * ((b6 * b6) >> 12)) >> 16;
  int32_t x3c = (x1c + x2c + 2) >> 2;
  uint32_t b4 = (calib->ac4 * (uint32_t)(x3c + 32768)) >> 15;
  uint32_t b7 = ((uint32_t)up - (uint32_t)b3) * (50000 >> oss);
  int32_t pa  = (b7 < 0x80000000) ? ((b7 * 2) / b4) : ((b7 / b4) * 2);
  int32_t x1d = (pa >> 8) * (pa >> 8);
  int32_t x1e = (x1d * 3038) >> 16;
  int32_t x2e = (-7357 * pa) >> 16;
  int32_t pb  = pa + ((x1e + x2e + 3791) >> 4);

  res->have_result = true;
  res->temperature = t / 10.0;  /* deci°C to °C */
  res->pressure    = pb;
}
