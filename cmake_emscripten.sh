#!/bin/sh

cmake -DCMAKE_TOOLCHAIN_FILE=./CMakeModules/Emscripten.cmake -DCMAKE_BUILD_TYPE=Debug -G "Unix Makefiles" .


