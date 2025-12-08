cmake_minimum_required(VERSION 3.16)

# SHM Guider Library
# Pure C/C++ library for shared memory equipment communication with external clients
# Supports camera selection, mount control, and other equipment
# No dependencies on PHD2 libraries or wxWidgets

project(shm_guider_lib)

# Source files for the library
set(SHM_GUIDER_SOURCES
  shm_camera.cpp
  shm_mount.cpp
  shm_camera_config.c
  shm_camera.h
  shm_mount.h
  shm_camera_config.h
)

# Create static library
add_library(shm_guider STATIC ${SHM_GUIDER_SOURCES})

# Create shared library
add_library(shm_guider_shared SHARED ${SHM_GUIDER_SOURCES})

# Set public include directory for both
target_include_directories(shm_guider PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(shm_guider_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link with POSIX real-time library
target_link_libraries(shm_guider PUBLIC rt)
target_link_libraries(shm_guider_shared PUBLIC rt)

# Enable position-independent code for potential shared library use
set_target_properties(shm_guider PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  C_STANDARD 99
  CXX_STANDARD 11
)

set_target_properties(shm_guider_shared PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  C_STANDARD 99
  CXX_STANDARD 11
  VERSION 1.0
  SOVERSION 1
  OUTPUT_NAME shm_guider
)

# Install shared and static libraries
install(TARGETS shm_guider shm_guider_shared
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)

# Install headers
install(FILES 
  shm_camera.h
  shm_mount.h
  shm_camera_config.h
  shm_guider.h
  DESTINATION include/shm-guider
)
