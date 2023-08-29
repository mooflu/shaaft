include(${PROJECT_SOURCE_DIR}/utilscmake/FindExtras.cmake)

set(CMAKE_FIND_DEBUG_MODE TRUE)

if(NOT EMSCRIPTEN)
    find_package(ZLIB REQUIRED)
    find_package(PNG REQUIRED)
    #find_package(JPEG REQUIRED)
    #find_package(Freetype REQUIRED)
    find_package(SDL2 REQUIRED)
    find_package(SDL2_mixer REQUIRED)
    find_package(SDL2_image REQUIRED)
    find_package(PhysFS REQUIRED)
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
endif()

set(EXTRA_LIBRARIES "")
if(WIN32)
	set(EXTRA_LIBRARIES
        imm32.lib
        winmm.lib
        version.lib
    )
endif()
if(APPLE)
	FIND_LIBRARY(CARBON_LIBRARY Carbon)
	FIND_LIBRARY(FF_LIBRARY ForceFeedback)
	FIND_LIBRARY(IOKIT_LIBRARY IOKit)
	FIND_LIBRARY(COREAUDIO_LIBRARY CoreAudio)
	FIND_LIBRARY(COREVIDEO_LIBRARY CoreVideo)
	FIND_LIBRARY(AUDIOTOOLBOX_LIBRARY AudioToolbox)
	FIND_LIBRARY(AUDIOUNIT_LIBRARY AudioUnit)
	FIND_LIBRARY(ICONV_LIBRARY iconv)

	set(EXTRA_LIBRARIES
		${CARBON_LIBRARY}
		${FF_LIBRARY}
		${IOKIT_LIBRARY}
		${COREAUDIO_LIBRARY}
		${COREVIDEO_LIBRARY}
		${AUDIOTOOLBOX_LIBRARY}
		${AUDIOUNIT_LIBRARY}
		${ICONV_LIBRARY}
	)
endif()

include_directories(${CMAKE_PREFIX_PATH}/include)
include_directories(${SDL2_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/utils)
include_directories(${PROJECT_SOURCE_DIR}/utilsfs)
include_directories(${PROJECT_SOURCE_DIR}/utilsgl)
include_directories(${PROJECT_SOURCE_DIR}/utilssdl)
include_directories(${PROJECT_SOURCE_DIR}/utilsrandom)
include_directories(${PROJECT_SOURCE_DIR}/tinyxml)
include_directories(${PROJECT_SOURCE_DIR}/miniyaml)
include_directories(${PROJECT_SOURCE_DIR}/) # vmmlib/vector.hpp

file(GLOB GAME_SRC *.cpp)
file(GLOB GAME_HEADERS *.hpp)

add_executable(shaaft WIN32 MACOSX_BUNDLE ${GAME_SRC} ${GAME_HEADERS})

if(EMSCRIPTEN)
    message(STATUS "Setting compilation target to EMSCRIPTEN")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    string(REGEX REPLACE "\n *" " " EMSCRIPTEN_LINK_FLAGS "
        -s WASM=1
        -Oz
        -s USE_WEBGL2=1
        -s USE_SDL=2
        -s USE_SDL_IMAGE=2
        -s SDL2_IMAGE_FORMATS='[\"png\"]'
        -s USE_ZLIB=1
        -s USE_LIBPNG=1
        -s USE_ICU=1
        -s ALLOW_MEMORY_GROWTH=1
        -s EXPORTED_FUNCTIONS='[_main]'
        -s EXPORTED_RUNTIME_METHODS=['UTF8ToString']
        --preload-file ${CMAKE_SOURCE_DIR}/resource.dat@resource.dat
		-lidbfs.js
    ")
# -flto
# -s BINARYEN_METHOD='native-wasm'
# --emrun
# -O2
# --llvm-lto 1

#-s USE_OGG=1
#-s USE_VORBIS=1
#        --preload-file ${CMAKE_SOURCE_DIR}/data@data
#        --closure 1
#        -s FULL_ES3=1
#        -s LEGACY_GL_EMULATION=1

    set_target_properties(shaaft PROPERTIES LINK_FLAGS ${EMSCRIPTEN_LINK_FLAGS})
endif()

if(NOT EMSCRIPTEN)
    set(EXTRA_LIBRARIES_2
        ${PHYSFS_LIBRARY}
        ${OGG_LIBRARY}
        ${VORBIS_LIBRARY}
        ${VORBISFILE_LIBRARY}
        GLEW::glew_s
    )
else()
	#FIND_LIBRARY(PHYSFS_LIBRARY physfs)
    set(EXTRA_LIBRARIES_2
        "${PROJECT_SOURCE_DIR}/oem.emscripten/lib/libphysfs.a"
    )
endif()

target_link_libraries(shaaft utils utilsrandom utilssdl utilsfs utilsgl tinyxml miniyaml
SDL2_mixer::SDL2_mixer-static
SDL2_image::SDL2_image-static
${SDL2_LIBRARIES}
${PNG_LIBRARY}
${ZLIB_LIBRARY}
${OPENGL_LIBRARY}
${EXTRA_LIBRARIES}
${EXTRA_LIBRARIES_2}
${CMAKE_DL_LIBS}
)
