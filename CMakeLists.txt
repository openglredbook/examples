cmake_minimum_required (VERSION 2.6)

project (vermilion9)

LINK_DIRECTORIES( ${CMAKE_SOURCE_DIR}/lib )

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Second, for multi-config builds (e.g. msvc)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/bin )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/lib )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/lib )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

set (CMAKE_DEBUG_POSTFIX "_d")

find_package(OpenGL REQUIRED)

if(WIN32)
set(COMMON_LIBS vermilion ${OPENGL_LIBRARIES} optimized glfw3 debug glfw3_d)
elseif (UNIX)
set(COMMON_LIBS vermilion ${OPENGL_LIBRARIES} glfw ${GLFW_LIBRARIES} GL rt dl)
else()
set(COMMON_LIBS vermilion)
endif()

set(COMMON_HEADERS include/vgl.h include/vapp.h)

set(COMMON_LIBS ${COMMON_LIBS} ${EXTRA_LIBS})

add_library(vermilion
            lib/gl3w.c
            lib/LoadShaders.cpp
            lib/targa.cpp
            lib/vdds.cpp
            lib/loadtexture.cpp
            lib/vermilion.cpp
            lib/vbm.cpp
)

set(RUN_DIR ${PROJECT_SOURCE_DIR}/bin)

set(EXAMPLES
  01-keypress
  01-triangles
  03-drawcommands
  03-instancing
  03-instancing2
  03-instancing3
  03-pointsprites
  03-primitiverestart
  03-xfb
  04-gouraud
  04-gouraud-float
  04-shadowmap
  06-cubemap
  06-load-texture
  06-mipfilters
  06-multitexture
  06-statictexture
  06-texturewrapping
  06-volumetexturing
  08-lightmodels
  09-simpletess
  09-teapot
  10-draw-xfb
  10-fur
  10-viewport-array
  11-doublewrite
  11-oit
  11-overdrawcount
  12-imageprocessing
  12-particlesimulator
  12-raytracer
  12-simplecompute
)

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

foreach(EXAMPLE ${EXAMPLES})
  add_executable(${EXAMPLE} WIN32 src/${EXAMPLE}/${EXAMPLE}.cpp ${COMMON_HEADERS})
  set_property(TARGET ${EXAMPLE} PROPERTY DEBUG_POSTFIX _d)
  target_link_libraries(${EXAMPLE} ${COMMON_LIBS})
  if(MSVC)
    configure_file(${PROJECT_SOURCE_DIR}/build/templates/vs2013.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/${EXAMPLE}.vcxproj.user @ONLY)
  endif(MSVC)
endforeach(EXAMPLE)

IF (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_LINUX")
ENDIF (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

include_directories( include )
include_directories(lib/glfw/include)

add_subdirectory(lib/glfw)

ADD_CUSTOM_TARGET(debug ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE:STRING=Debug ${PROJECT_SOURCE_DIR})
ADD_CUSTOM_TARGET(release ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE:STRING=Release ${PROJECT_SOURCE_DIR})
