cmake_minimum_required( VERSION 3.10 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

add_subdirectory(_grfx/Cube/proj/cmake)
add_subdirectory(_grfx/CubeHLSL/proj/cmake)
add_subdirectory(_grfx/CubeMapping/proj/cmake)
add_subdirectory(_grfx/NormalMapping/proj/cmake)
add_subdirectory(_grfx/NormalMappingBasic/proj/cmake)