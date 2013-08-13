project (i2c-sensors)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Werror -Wall")

add_executable (main-test main.c i2c-sensors.c bmp085.c l3gd20.c
                          lsm303dlhc-acc.c lsm303dlhc-mag.c)
