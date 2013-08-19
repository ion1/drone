cmake_minimum_required (VERSION 2.8)

project (DroneTest)

set (CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -std=gnu99 -Wall -g -O2")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -O2")

set (XPLANE_SDK_PATH "${CMAKE_SOURCE_DIR}/SDK" CACHE PATH "X-Plane SDK path")
set (XPLANE_PATH "${CMAKE_SOURCE_DIR}/X-Plane" CACHE PATH "X-Plane path")

foreach (dir Widgets Wrappers XPLM)
  include_directories ("${XPLANE_SDK_PATH}/CHeaders/${dir}")
endforeach ()

macro (add_xplane_plugin target_name sources)
  foreach (bits 32 64)
    set (t_b_name "${target_name}-${bits}")
    add_library ("${t_b_name}" SHARED ${sources})
    set_target_properties ("${t_b_name}" PROPERTIES PREFIX "" SUFFIX ".xpl")
    set_target_properties ("${t_b_name}" PROPERTIES COMPILE_DEFINITIONS
      "LIN=1;XPLM210=1")
    set_target_properties ("${t_b_name}" PROPERTIES COMPILE_FLAGS
      "-m${bits} -fno-stack-protector -fPIC -fvisibility=hidden")
    set_target_properties ("${t_b_name}" PROPERTIES LINK_FLAGS
      "-m${bits} -nodefaultlibs -Wl,--version-script=${CMAKE_SOURCE_DIR}/version-script")

    install (
      TARGETS "${t_b_name}"
      DESTINATION "${XPLANE_PATH}/Resources/plugins")
  endforeach ()
endmacro ()

add_xplane_plugin (DroneTest drone-test.c)
