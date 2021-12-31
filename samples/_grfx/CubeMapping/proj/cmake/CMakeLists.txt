cmake_minimum_required( VERSION 3.10 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

project( grfx-CubeMapping )

get_filename_component( APP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../" ABSOLUTE )

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

ci_make_app(
	APP_NAME    "CubeMapping"
	SOURCES     ${APP_PATH}/src/CubeMappingApp.cpp
	CINDER_PATH ${CINDER_PATH}
    FOLDER      "cinder-grfx/samples"
)

target_link_libraries(
    CubeMapping 
    PUBLIC cinder-grfx-renderers
)
