#!/bin/bash

mkdir -p afl-build || exit 1
cd afl-build || exit 1
#cleanup
rm -r -- *

CC=afl-clang-fast cmake ../.. -DENABLE_FUZZING=On -DENABLE_SANITIZERS=On -DBUILD_SHARED_LIBS=Off
make afl
