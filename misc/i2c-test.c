#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* Uses NULL from stddef.h without including it. */
#include <linux/i2c-dev.h>

#define BMP085_ADDR 0x77

#define BMP085_CALIB_REG 0xaa

#define BMP085_CTRL_REG  0xf4
#define BMP085_CTRL_TEMP 0x2e
#define BMP085_CTRL_PRES 0x34

#define BMP085_DATA 0xf6

typedef struct {
  int16_t  ac1, ac2, ac3;
  uint16_t ac4, ac5, ac6;
  int16_t  b1, b2, mb, mc, md;
} bmp085_calib_t;

#define BMP085_CALIB_EXAMPLE \
  ((bmp085_calib_t){ 408, -72, -14383, 32741, 32757, 23153 \
                   , 6190, 4, -32768, -8711, 2868 \
                   })
#define BMP085_UT_EXAMPLE  ((int32_t)27898)
#define BMP085_OSS_EXAMPLE ((int16_t)0)
#define BMP085_UP_EXAMPLE  ((int32_t)23843)

void
bmp085_read_calib (const int fd, bmp085_calib_t *const calib);

void
bmp085_measure_temp_start (const int fd);
uint32_t
bmp085_measure_temp_finish (const int fd);
void
bmp085_measure_pres_start (const int fd, const uint16_t oss);
uint32_t
bmp085_measure_pres_finish (const int fd, const uint16_t oss);

void
bmp085_calculate ( const bmp085_calib_t *const calib
                 , const int32_t ut
                 , const int16_t oss
                 , const int32_t up
                 , int32_t *const t_out
                 , int32_t *const p_out
                 );

double
bmp085_altitude (const double p_sea, const double p);

void
sleep_ns (uint32_t ns);

uint8_t
i2c_read_u8 (const int fd, uint8_t command);
uint16_t
i2c_read_u16 (const int fd, uint8_t command);
uint32_t
i2c_read_u24 (const int fd, uint8_t command);
uint32_t
i2c_read_u32 (const int fd, uint8_t command);

void
i2c_write_u8 (const int fd, uint8_t command, uint8_t data);

int
main (void)
{
  /* http://weather.noaa.gov/pub/data/observations/metar/decoded/EFTP.TXT */
  /* http://www.aviationweather.gov/adds/metars/?station_ids=EFTP&std_trans=standard&chk_metars=on&hoursStr=past+36+hours&chk_tafs=on&submitmet=Submit */
  int32_t p_sea = 100400;

  int fd;
  if ((fd = open ("/dev/i2c-1", O_RDWR)) < 0)
    error_at_line (1, errno, __FILE__, __LINE__, "open failed");

  if (ioctl (fd, I2C_SLAVE, 0x77) < 0)
    error_at_line (1, errno, __FILE__, __LINE__, "ioctl failed");

  bmp085_calib_t calib = BMP085_CALIB_EXAMPLE;
  bmp085_read_calib (fd, &calib);

  printf( "ac1=%d ac2=%d ac3=%d ac4=%u ac5=%u ac6=%u "
          "b1=%d b2=%d mb=%d mc=%d md=%d\n"
        , calib.ac1, calib.ac2, calib.ac3, calib.ac4, calib.ac5, calib.ac6
        , calib.b1, calib.b2, calib.mb, calib.mc, calib.md
        );

  int32_t ut, up;
  int16_t oss = 3;
  bmp085_measure_temp_start (fd);
  sleep_ns (4500000);
  ut = bmp085_measure_temp_finish (fd);
  bmp085_measure_pres_start (fd, oss);
  sleep_ns (25500000);
  up = bmp085_measure_pres_finish (fd, oss);

  int32_t t, p;
  bmp085_calculate (&calib, ut, oss, up, &t, &p);
  double alt = bmp085_altitude (p_sea, p);
  printf ("t=%d p=%d\n", t, p);
  printf ("p_sea=%d alt=%.2f\n", p_sea, alt);

  close (fd);

  return 0;
}

void
bmp085_read_calib (const int fd, bmp085_calib_t *const calib)
{
  calib->ac1 = i2c_read_u16(fd, BMP085_CALIB_REG+0x00);
  calib->ac2 = i2c_read_u16(fd, BMP085_CALIB_REG+0x02);
  calib->ac3 = i2c_read_u16(fd, BMP085_CALIB_REG+0x04);
  calib->ac4 = i2c_read_u16(fd, BMP085_CALIB_REG+0x06);
  calib->ac5 = i2c_read_u16(fd, BMP085_CALIB_REG+0x08);
  calib->ac6 = i2c_read_u16(fd, BMP085_CALIB_REG+0x0a);
  calib->b1  = i2c_read_u16(fd, BMP085_CALIB_REG+0x0c);
  calib->b2  = i2c_read_u16(fd, BMP085_CALIB_REG+0x0e);
  calib->mb  = i2c_read_u16(fd, BMP085_CALIB_REG+0x10);
  calib->mc  = i2c_read_u16(fd, BMP085_CALIB_REG+0x12);
  calib->md  = i2c_read_u16(fd, BMP085_CALIB_REG+0x14);
}

void
bmp085_measure_temp_start (const int fd) {
  i2c_write_u8 (fd, BMP085_CTRL_REG, BMP085_CTRL_TEMP);
}

uint32_t
bmp085_measure_temp_finish (const int fd)
{
  return i2c_read_u16 (fd, BMP085_DATA);
}

void
bmp085_measure_pres_start (const int fd, const uint16_t oss)
{
  i2c_write_u8 (fd, BMP085_CTRL_REG, BMP085_CTRL_PRES + (oss<<6));
}

uint32_t
bmp085_measure_pres_finish (const int fd, const uint16_t oss)
{
  uint32_t msb  = i2c_read_u8 (fd, BMP085_DATA+0)
         , lsb  = i2c_read_u8 (fd, BMP085_DATA+1)
         , xlsb = i2c_read_u8 (fd, BMP085_DATA+2)
         ;
  return ((msb<<16) | (lsb<<8) | xlsb) >> (8-oss);
}

void
bmp085_calculate ( const bmp085_calib_t *const calib
                 , const int32_t ut
                 , const int16_t oss
                 , const int32_t up
                 , int32_t *const t_out
                 , int32_t *const p_out
                 )
{
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

  *t_out = t;
  *p_out = pb;
}

inline double
bmp085_altitude (const double p_sea, const double p)
{
  return 44330 * (1 - pow (p/p_sea, 1/5.255));
}

void
sleep_ns (uint32_t ns)
{
  struct timespec t;
  t.tv_sec  = ns / 1000000000;
  t.tv_nsec = ns % 1000000000;
  int err = clock_nanosleep (CLOCK_MONOTONIC, 0, &t, NULL);
  if (err != 0)
    error_at_line (1, err, __FILE__, __LINE__, "clock_nanosleep failed");
}

uint8_t
i2c_read_u8 (const int fd, uint8_t command)
{
  int32_t b = i2c_smbus_read_byte_data (fd, command);
  if (b < 0)
    error_at_line ( 1, errno, __FILE__, __LINE__
                  , "i2c_smbus_read_byte_data failed"
                  );
  return b;
}

uint16_t
i2c_read_u16 (const int fd, uint8_t command)
{
  int32_t b0 = i2c_read_u8 (fd, command+0)
        , b1 = i2c_read_u8 (fd, command+1)
        ;
  return (b0<<8) | b1;
}

uint32_t
i2c_read_u24 (const int fd, uint8_t command)
{
  int32_t b0 = i2c_read_u8 (fd, command+0)
        , b1 = i2c_read_u8 (fd, command+1)
        , b2 = i2c_read_u8 (fd, command+2)
        ;
  return (b0<<16) | (b1<<8) | b2;
}

uint32_t
i2c_read_u32 (const int fd, uint8_t command)
{
  int32_t b0 = i2c_read_u8 (fd, command+0)
        , b1 = i2c_read_u8 (fd, command+1)
        , b2 = i2c_read_u8 (fd, command+2)
        , b3 = i2c_read_u8 (fd, command+3)
        ;
  return (b0<<24) | (b1<<16) | (b2<<8) | b3;
}

void
i2c_write_u8 (const int fd, uint8_t command, uint8_t data)
{
  int32_t res = i2c_smbus_write_byte_data (fd, command, data);
  if (res < 0)
    error_at_line ( 1, errno, __FILE__, __LINE__
                  , "i2c_smbus_write_byte_data failed"
                  );
}
