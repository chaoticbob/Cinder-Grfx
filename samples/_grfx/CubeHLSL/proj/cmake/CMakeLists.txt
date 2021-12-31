cmake_minimum_required( VERSION 3.10 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

project( grfx-CubeHLSL )

get_filename_component( APP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../" ABSOLUTE )

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

ci_make_app(
	APP_NAME    "CubeHLSL"
	SOURCES     ${APP_PATH}/src/CubeApp.cpp
	CINDER_PATH ${CINDER_PATH}
    FOLDER      "cinder-grfx/samples"
)

target_link_libraries(
    CubeHLSL 
    PUBLIC cinder-grfx-renderers
)