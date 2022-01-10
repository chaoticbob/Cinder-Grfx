cmake_minimum_required( VERSION 3.10 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

project( NormalMappingBasic )

#get_filename_component( CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE )
get_filename_component( APP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../" ABSOLUTE )

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

ci_make_app(
	APP_NAME    "NormalMappingBasic"
	SOURCES     ${APP_PATH}/src/NormalMappingBasicApp.cpp
	CINDER_PATH ${CINDER_PATH}
    FOLDER      "cinder/samples/_opengl"
    FOLDER      "cinder-grfx/samples"
)

target_link_libraries(
    NormalMappingBasic 
    PUBLIC cinder-grfx-renderers
)
