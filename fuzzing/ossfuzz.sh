#!/bin/bash -eu

# This script is meant to be run by
# https://github.com/google/oss-fuzz/blob/master/projects/cjson/Dockerfile

mkdir build
cd build
cmake -DBUILD_SHARED_LIBS=OFF -DENABLE_CJSON_TEST=OFF ..
make -j$(nproc)

$CXX $CXXFLAGS $SRC/cjson/fuzzing/cjson_read_fuzzer.c -I. \
    -o $OUT/cjson_read_fuzzer \
    $LIB_FUZZING_ENGINE $SRC/cjson/build/libcjson.a

find $SRC/cjson/fuzzing/inputs -name "*" | \
     xargs zip $OUT/cjson_read_fuzzer_seed_corpus.zip

cp $SRC/cjson/fuzzing/json.dict $OUT/cjson_read_fuzzer.dict

# The shipped inputs under fuzzing/inputs are formatted for fuzzing/afl.c
# (letters 'b'/'u'/'f' as flags, newline terminated) and are rejected by
# this libFuzzer harness, which requires four '0'/'1' flag characters and
# a trailing NUL byte. Generate properly framed structured seeds and
# append them to the existing corpus zip (nothing removed).
python3 $SRC/cjson/fuzzing/gen_read_seeds.py $SRC/cjson/generated_read_seeds
zip -j $OUT/cjson_read_fuzzer_seed_corpus.zip $SRC/cjson/generated_read_seeds/*
