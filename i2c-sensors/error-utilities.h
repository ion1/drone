#ifndef INCLUDE_ERROR_UTILITIES_H
#define INCLUDE_ERROR_UTILITIES_H

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define ERROR_SIZE 1000

typedef struct {
  size_t size;
  char message[ERROR_SIZE];
} error_t;

#define ERROR_DECLARE(name) \
  error_t name = { ERROR_SIZE, "" }

static inline void
error_copy (error_t *const err, const char *const str)
{
  strncpy (err->message, str, err->size);
  err->message[err->size - 1] = '\0';
}

static inline void
error_printf (error_t *const err, const char *const format, ...)
  __attribute__ ((format (printf, 2, 3)));

static inline void
error_printf (error_t *const err, const char *const format, ...)
{
  va_list args;
  va_start (args, format);
  vsnprintf (err->message, err->size, format, args);
  va_end (args);
}

static inline void
error_prefix (error_t *const err, const char *const prefix)
{
  ERROR_DECLARE (orig);
  error_copy (&orig, err->message);
  error_printf (err, "%s: %s", prefix, orig.message);
}

static inline void
error_prefix_printf (error_t *const err, const char *const format, ...)
  __attribute__ ((format (printf, 2, 3)));

static inline void
error_prefix_printf (error_t *const err, const char *const format, ...)
{
  ERROR_DECLARE (prefix);
  va_list args;
  va_start (args, format);
  vsnprintf (prefix.message, prefix.size, format, args);
  va_end (args);

  error_prefix (err, prefix.message);
}

static inline void
error_strerror (error_t *const err, int errnum)
{
  /* Assign to an int to make sure we have the XSI-compliant strerror_r. */
  int res = strerror_r (errnum, err->message, err->size);
  (void)res;
}

#endif /* INCLUDE_ERROR_UTILITIES_H */
