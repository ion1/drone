#ifndef INCLUDE_L3GD20_H
#define INCLUDE_L3GD20_H

#include <stdbool.h>

typedef struct l3gd20 l3gd20_t;

l3gd20_t *
l3gd20_new (const int i2c_fd, char **errstr, int *err);

void
l3gd20_free (l3gd20_t *const l3gd20);

bool
l3gd20_run ( l3gd20_t *const l3gd20, double *const x_out
           , double *const y_out, double *const z_out );

#endif /* INCLUDE_L3GD20_H */
