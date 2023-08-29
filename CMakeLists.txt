cmake_minimum_required(VERSION 3.5)

project(shaaft C CXX)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/utilscmake")

add_definitions(-DPACKAGE="Shaaft" -DVERSION="2.1" -DSTATIC_LIB)

if(EMSCRIPTEN)
    string(REGEX REPLACE "\n *" " " EMSCRIPTEN_FLAGS "
        -fno-rtti
        -Oz
        -s WASM=1
        -s USE_WEBGL2=1
        -s USE_SDL=2
        -s USE_SDL_IMAGE=2
        -s SDL2_IMAGE_FORMATS='[\"png\"]'
        -s USE_SDL_MIXER=2
        -s USE_ZLIB=1
        -s USE_LIBPNG=1
        -s USE_ICU=1
    ")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EMSCRIPTEN_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${PROJECT_SOURCE_DIR}/3rdparty/glm -I${PROJECT_SOURCE_DIR}/oem.emscripten/include/")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${PROJECT_SOURCE_DIR}/3rdparty/glm -I${PROJECT_SOURCE_DIR}/oem/include/")
endif()
#-s USE_OGG=1
#-s USE_VORBIS=1
#-O2
#-fno-exceptions
#-s FULL_ES3=1

set(CMAKE_DEBUG_POSTFIX "_d")
set(CMAKE_CXX_STANDARD 11)

include(${PROJECT_SOURCE_DIR}/utilscmake/PlatformSetup.cmake)

add_subdirectory(${PROJECT_SOURCE_DIR}/utils ${CMAKE_BINARY_DIR}/utils)
add_subdirectory(${PROJECT_SOURCE_DIR}/utilsrandom ${CMAKE_BINARY_DIR}/utilsrandom)
add_subdirectory(${PROJECT_SOURCE_DIR}/utilssdl ${CMAKE_BINARY_DIR}/utilssdl)
add_subdirectory(${PROJECT_SOURCE_DIR}/utilsgl ${CMAKE_BINARY_DIR}/utilsgl)
add_subdirectory(${PROJECT_SOURCE_DIR}/utilsfs ${CMAKE_BINARY_DIR}/utilsfs)
add_subdirectory(${PROJECT_SOURCE_DIR}/tinyxml ${CMAKE_BINARY_DIR}/tinyxml)
add_subdirectory(${PROJECT_SOURCE_DIR}/miniyaml ${CMAKE_BINARY_DIR}/miniyaml)
add_subdirectory(${PROJECT_SOURCE_DIR}/game)
