cmake_minimum_required( VERSION 3.10 FATAL_ERROR )
cmake_policy(SET CMP0091 NEW)

set( CMAKE_VERBOSE_MAKEFILE ON )

project( cinder-grfx )

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# CINDER_PATH needs to be passed in from the cmake command args
if (NOT CINDER_PATH)
    message(FATAL_ERROR "CINDER_PATH not set")
endif()
# Get absolute path
if (NOT IS_ABSOLUTE CINDER_PATH)
    get_filename_component(CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${CINDER_PATH}" ABSOLUTE)
endif()
# Look for include/cinder/Cinder.h to verify it's really Cinder
set(CINDER_H_PATH "${CINDER_PATH}/include/cinder/Cinder.h")
if (NOT EXISTS ${CINDER_H_PATH})
    message(FATAL_ERROR "Could not find Cinder.h at ${CINDER_H_PATH} to verify that ${CINDER_PATH} is a Cinder source tree")
endif()
# Let user know what CINDER_PATH is
message("-- Found Cinder at: ${CINDER_PATH}")

# CINDER_GRFX_PATH 
set(CINDER_GRFX_PATH ${CMAKE_CURRENT_SOURCE_DIR})

# ------------------------------------------------------------------------------
# Multi-process build for MSVC
# ------------------------------------------------------------------------------
if(MSVC)
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} /MP")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

# Match Cinder's /MT and /MTd for Windows. 
# This requires cmake_policy(SET CMP0091 NEW) which is
# at the top of this file.
#
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Add glslang for Vulkan GLSL compilation
add_subdirectory(third_party/glslang)

# Cinder dependency
add_subdirectory(${CINDER_PATH} cinder-build-dir-here)

# Source
add_subdirectory(src)

# Samples
add_subdirectory(samples)