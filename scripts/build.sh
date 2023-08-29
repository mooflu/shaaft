#!/bin/bash -x

export PROJ_FOLDER=`pwd`
export OEM="oem"

case "$(uname -sr)" in
   CYGWIN*)
     echo 'MS Windows'
     export PROJ_FOLDER=`cygpath -w ${PROJ_FOLDER}`
     ;;

   *)
     ;;
esac

if [ ! -f resource.dat ]; then
    pushd data
    zip -9r ../resource.dat .
    popd
fi

echo proj folder: ${PROJ_FOLDER}

mkdir -p 3rdparty
pushd 3rdparty
if [ ! -d glm ]; then
	git clone --depth 1 --branch 0.9.9.8 https://github.com/g-truc/glm.git
fi
if [ ! -d physfs ]; then
	git clone --depth 1 --branch release-3.2.0 https://github.com/icculus/physfs.git
fi

    pushd physfs
    cmake \
        -DCMAKE_INSTALL_PREFIX:PATH=${PROJ_FOLDER}/${OEM} \
        -DCMAKE_PREFIX_PATH:PATH=${PROJ_FOLDER}/${OEM} \
        -DPHYSFS_BUILD_SHARED=False \
        -DPHYSFS_ARCHIVE_7Z:BOOL=ON \
        -DPHYSFS_ARCHIVE_ZIP=ON \
        -DPHYSFS_ARCHIVE_GRP=OFF \
        -DPHYSFS_ARCHIVE_WAD=OFF \
        -DPHYSFS_ARCHIVE_CSM=OFF \
        -DPHYSFS_ARCHIVE_HOG=OFF \
        -DPHYSFS_ARCHIVE_MVL=OFF \
        -DPHYSFS_ARCHIVE_QPAK=OFF \
        -DPHYSFS_ARCHIVE_SLB=OFF \
        -DPHYSFS_ARCHIVE_ISO9660=OFF \
        -DPHYSFS_ARCHIVE_VDF=OFF \
        .
    cmake --build . --config Release
    cmake --install .
    popd

if [ ! -d zlib ]; then
	git clone --depth 1 --branch v1.3 https://github.com/madler/zlib.git
fi

    pushd zlib
    cmake \
        -DCMAKE_INSTALL_PREFIX:PATH=${PROJ_FOLDER}/${OEM} \
        -DCMAKE_PREFIX_PATH:PATH=${PROJ_FOLDER}/${OEM} \
        -DBUILD_SHARED_LIBS:BOOL=OFF \
        .
    cmake --build . --config Release
    cmake --install .
    # zlib doesn't seem to honour BUILD_SHARED_LIBS false
    rm -f ${PROJ_FOLDER}/${OEM}/lib/libz.so*
    rm -f ${PROJ_FOLDER}/${OEM}/lib/zlib.dll
    rm -f ${PROJ_FOLDER}/${OEM}/lib/zlib.lib
    popd

if [ ! -d libpng ]; then
    if [ ! -f libpng-1.6.40.tar.gz ]; then
        wget https://download.sourceforge.net/libpng/libpng-1.6.40.tar.gz
    fi
    tar xf libpng-1.6.40.tar.gz
    mv libpng-1.6.40 libpng
fi

    pushd libpng
    cmake \
        -DCMAKE_INSTALL_PREFIX:PATH=${PROJ_FOLDER}/${OEM} \
        -DCMAKE_PREFIX_PATH:PATH=${PROJ_FOLDER}/${OEM} \
        -DBUILD_SHARED_LIBS:BOOL=OFF \
        -DPNG_SHARED:BOOL=OFF \
        -DPNG_STATIC:BOOL=ON \
        .
    cmake --build . --config Release
    cmake --install .
    popd

if [ ! -d glew ]; then
	git clone --depth 1 --branch glew-2.2.0 https://github.com/nigels-com/glew.git
fi
    pushd glew/auto
    make
    popd

    pushd glew/build
    cmake \
        -DCMAKE_INSTALL_PREFIX:PATH=${PROJ_FOLDER}/${OEM} \
        -DCMAKE_PREFIX_PATH:PATH=${PROJ_FOLDER}/${OEM} \
        -DBUILD_SHARED_LIBS:BOOL=OFF \
        -BUILD_UTILS:BOOL=OFF \
        ./cmake
    cmake --build . --config Release
    cmake --install .
    popd

if [ ! -d SDL ]; then
	git clone --depth 1 --branch release-2.28.2 https://github.com/libsdl-org/SDL.git
fi

    pushd SDL
        mkdir -p build
        pushd build
        cmake -S .. -B . \
            -DCMAKE_INSTALL_PREFIX:PATH=${PROJ_FOLDER}/${OEM} \
            -DCMAKE_PREFIX_PATH:PATH=${PROJ_FOLDER}/${OEM} \
            -DBUILD_SHARED_LIBS:BOOL=OFF \
            -DSDL_SHARED:BOOL=OFF
        cmake --build . --config Release
        cmake --install .
        popd
    popd

if [ ! -d SDL_image ]; then
	git clone --depth 1 --branch release-2.6.3 https://github.com/libsdl-org/SDL_image.git
fi

    pushd SDL_image
        mkdir -p build
        pushd build
        cmake -S .. -B . \
            -DCMAKE_INSTALL_PREFIX:PATH=${PROJ_FOLDER}/${OEM} \
            -DCMAKE_PREFIX_PATH:PATH=${PROJ_FOLDER}/${OEM} \
            -DBUILD_SHARED_LIBS:BOOL=OFF \
            -DSDL2IMAGE_AVIF:BOOL=OFF \
            -DSDL2IMAGE_BMP:BOOL=OFF \
            -DSDL2IMAGE_GIF:BOOL=OFF \
            -DSDL2IMAGE_JPG:BOOL=OFF \
            -DSDL2IMAGE_JXL:BOOL=OFF \
            -DSDL2IMAGE_LBM:BOOL=OFF \
            -DSDL2IMAGE_PCX:BOOL=OFF \
            -DSDL2IMAGE_PNM:BOOL=OFF \
            -DSDL2IMAGE_QOI:BOOL=OFF \
            -DSDL2IMAGE_SVG:BOOL=OFF \
            -DSDL2IMAGE_TGA:BOOL=OFF \
            -DSDL2IMAGE_TIF:BOOL=OFF \
            -DSDL2IMAGE_WEBP:BOOL=OFF \
            -DSDL2IMAGE_XCF:BOOL=OFF \
            -DSDL2IMAGE_XPM:BOOL=OFF \
            -DSDL2IMAGE_XV:BOOL=OFF \
            -DSDL2IMAGE_SAMPLES:BOOL=OFF
        cmake --build . --config Release
        cmake --install .
        popd
    popd


if [ ! -d SDL_mixer ]; then
	git clone --depth 1 --branch release-2.6.3 https://github.com/libsdl-org/SDL_mixer.git
fi

    pushd SDL_mixer
        mkdir -p build
        pushd build
        cmake -S .. -B . \
            -DCMAKE_INSTALL_PREFIX:PATH=${PROJ_FOLDER}/${OEM} \
            -DCMAKE_PREFIX_PATH:PATH=${PROJ_FOLDER}/${OEM} \
            -DBUILD_SHARED_LIBS:BOOL=OFF \
            -DSDL2MIXER_OPUS:BOOL=OFF \
            -DSDL2MIXER_MIDI:BOOL=OFF \
            -DSDL2MIXER_MP3:BOOL=OFF \
            -DSDL2MIXER_MOD:BOOL=OFF \
            -DSDL2MIXER_GME:BOOL=OFF \
            -DSDL2MIXER_FLAC:BOOL=OFF \
            -DSDL2MIXER_CMD:BOOL=OFF \
            -DSDL2MIXER_SNDFILE:BOOL=OFF \
            -DSDL2MIXER_SAMPLES:BOOL=OFF
        cmake --build . --config Release
        cmake --install .
        popd
    popd

if [ ! -d libogg ]; then
    if [ ! -f libogg-1.3.5.tar.gz ]; then
        wget https://downloads.xiph.org/releases/ogg/libogg-1.3.5.tar.gz
    fi
    tar xf libogg-1.3.5.tar.gz
    mv libogg-1.3.5 libogg
fi

    pushd libogg
    cmake \
        -DCMAKE_INSTALL_PREFIX:PATH=${PROJ_FOLDER}/${OEM} \
        -DCMAKE_PREFIX_PATH:PATH=${PROJ_FOLDER}/${OEM} \
        -DBUILD_SHARED_LIBS:BOOL=OFF \
        .
    cmake --build . --config Release
    cmake --install .
    popd

if [ ! -d libvorbis ]; then
    if [ ! -f libvorbis-1.3.7.tar.gz ]; then
        wget https://downloads.xiph.org/releases/vorbis/libvorbis-1.3.7.tar.gz
    fi
    tar xf libvorbis-1.3.7.tar.gz
    mv libvorbis-1.3.7 libvorbis
fi

    pushd libvorbis
    cmake \
        -DCMAKE_INSTALL_PREFIX:PATH=${PROJ_FOLDER}/${OEM} \
        -DCMAKE_PREFIX_PATH:PATH=${PROJ_FOLDER}/${OEM} \
        -DBUILD_SHARED_LIBS:BOOL=OFF \
        .
    cmake --build . --config Release
    cmake --install .
    popd

popd

mkdir -p build
pushd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=${PROJ_FOLDER}/${OEM} -DCMAKE_PREFIX_PATH:PATH=${PROJ_FOLDER}/${OEM} -DBUILD_SHARED_LIBS:BOOL=OFF ..
cmake --build . --config Release
popd

