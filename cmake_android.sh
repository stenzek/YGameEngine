#!/bin/bash

BASEPATH="${BASH_SOURCE[0]}"
BASEPATH=$(dirname $BASEPATH)
echo BASEPATH=$BASEPATH

cmake -G"Unix Makefiles" \
      -DCMAKE_TOOLCHAIN_FILE=$BASEPATH/CMakeModules/android.toolchain.cmake \
      -DANDROID_ABI=armeabi-v7a \
      -DANDROID_NATIVE_API_LEVEL=android-19 \
      -DANDROID_STL=gnustl_shared \
      $BASEPATH

