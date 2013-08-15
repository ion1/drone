#ifndef INCLUDE_ERROR_UTILITIES_H
#define INCLUDE_ERROR_UTILITIES_H

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define ERROR_SIZE 1000

/* The message cursor points to the last byte (which must be initialized as
 * '\0') and is moved toward the start as functions insert strings to the
 * error. This way truncation will happen to the least important information.
 */
typedef struct {
  char buffer[ERROR_SIZE-1];
  char terminating_zero;
  char *message;
} error_t;

#define ERROR_DECLARE(name) \
  error_t name = { "", '\0', &name.terminating_zero }

static void
error_insertn (error_t *const err, const char *const str, size_t len)
{
  /* Copy chars from back to front. Stop whenever the start of either the error
   * buffer or the string is reached.
   */
  const char *strp = &str[len];

  for (; strp > str && err->message > err->buffer; --strp, --err->message)
    err->message[-1] = strp[-1];

  if (err->message == err->buffer && strp > str)
    /* str did not fit. Set the beginning of the message to "..." */
    for (int i = 0; i < 3 && err->buffer[i] != '\0'; ++i)
      err->buffer[i] = '.';
}

static inline void
error_insert (error_t *const err, const char *const str)
{
  error_insertn (err, str, strlen (str));
}

static inline void
error_printf (error_t *const err, const char *const format, ...)
  __attribute__ ((format (printf, 2, 3)));

static inline void
error_printf (error_t *const err, const char *const format, ...)
{
  char buf[ERROR_SIZE];
  int len;

  va_list args;
  va_start (args, format);
  len = vsnprintf (buf, ERROR_SIZE, format, args);
  va_end (args);

  error_insertn (err, buf, len);
}

static inline void
error_prefix (error_t *const err, const char *const prefix)
{
  error_insertn (err, ": ", 2);
  error_insert (err, prefix);
}

static inline void
error_prefix_printf (error_t *const err, const char *const format, ...)
  __attribute__ ((format (printf, 2, 3)));

static inline void
error_prefix_printf (error_t *const err, const char *const format, ...)
{
  char buf[ERROR_SIZE];
  int len;

  va_list args;
  va_start (args, format);
  len = vsnprintf (buf, ERROR_SIZE, format, args);
  va_end (args);

  error_insertn (err, ": ", 2);
  error_insertn (err, buf, len);
}

static inline void
error_strerror (error_t *const err, int errnum)
{
  char buf[200];

  /* Assign to an int to make sure we have the XSI-compliant strerror_r. */
  int res = strerror_r (errnum, buf, 200);
  (void)res;

  error_insert (err, buf);
}

static inline void
error_errno (error_t *const err)
{
  error_strerror (err, errno);
}

#endif /* INCLUDE_ERROR_UTILITIES_H */
