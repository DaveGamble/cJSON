#!/bin/bash

mkdir -p libfuzzer-build || exit 1
cd libfuzzer-build || exit 1
#cleanup
rm -r -- *

CC=clang cmake ../.. -DENABLE_FUZZING=On -DENABLE_SANITIZERS=On -DBUILD_SHARED_LIBS=Off -DCMAKE_BUILD_TYPE=Debug -DENABLE_LIBFUZZER=On
make fuzz-target
