#include <stdlib.h>

#include "error-utilities.h"

error_t *
error_new (void)
{
  error_t *err = malloc (sizeof (error_t));
  if (! err)
    return NULL;

  err->terminating_zero = '\0';
  err->message = &err->terminating_zero;

  return err;
}

void
error_free (error_t *const err)
{
  free (err);
}

void
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
