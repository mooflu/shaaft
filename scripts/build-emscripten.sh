#!/bin/bash

if [[ -z "${EMSDK}" ]]; then
    echo EMSDK not set
    exit
fi

export PROJ_FOLDER=`pwd`

if [ ! -f resource.dat ]; then
    pushd data
    zip -9r ../resource.dat .
    popd
fi

mkdir -p 3rdparty
pushd 3rdparty
if [ ! -d 3rdparty/glm ]; then
	git clone --depth 1 --branch 0.9.9.8 https://github.com/g-truc/glm.git
fi
if [ ! -d 3rdparty/physfs ]; then
	git clone --depth 1 --branch release-3.2.0 https://github.com/icculus/physfs.git
fi

    pushd physfs
    emcmake cmake \
        -DCMAKE_INSTALL_PREFIX:PATH=${PROJ_FOLDER}/oem.emscripten \
        -DCMAKE_PREFIX_PATH:PATH=${PROJ_FOLDER}/oem.emscripten \
        -DPHYSFS_BUILD_SHARED=False \
        -DCMAKE_BUILD_TYPE=MinSizeRel \
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
    emmake make VERBOSE=1 -j`nproc`
    emmake make VERBOSE=1 -j`nproc` install
    popd

popd

mkdir -p build
pushd build
emcmake cmake -DCMAKE_INSTALL_PREFIX:PATH=${PROJ_FOLDER}/oem.emscripten -DCMAKE_PREFIX_PATH:PATH=${PROJ_FOLDER}/oem.emscripten -DWASM=1 ..
emmake make VERBOSE=1 -j`nproc`
mkdir webapp
cp game/shaaft.* webapp
popd

