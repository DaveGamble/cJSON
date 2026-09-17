#!/bin/bash -eu

# This script is meant to be run by
# https://github.com/google/oss-fuzz/blob/master/projects/cjson/Dockerfile

mkdir build
cd build

cmake -DBUILD_SHARED_LIBS=OFF -DENABLE_CJSON_TEST=OFF ..
make -j"$(nproc)"

FUZZERS=(
    cjson_read_fuzzer
    cjson_create_fuzzer
    cjson_add_to_object_fuzzer
    cjson_delete_item_from_object_fuzzer
    cjson_replace_item_in_object_fuzzer
    cjson_duplicate_fuzzer
    cjson_compare_fuzzer
    cjson_print_preallocated_fuzzer
)

for fuzzer in "${FUZZERS[@]}"; do
    $CXX $CXXFLAGS "$SRC/cjson/fuzzing/${fuzzer}.c" -I. \
        -o "$OUT/${fuzzer}" \
        $LIB_FUZZING_ENGINE "$SRC/cjson/build/libcjson.a"

    if [ -d "$SRC/cjson/fuzzing/inputs" ] && \
       [ -n "$(find "$SRC/cjson/fuzzing/inputs" -type f -print -quit)" ]; then
        find "$SRC/cjson/fuzzing/inputs" -type f | \
            zip -q -@ "$OUT/${fuzzer}_seed_corpus.zip"
    fi

    if [ -f "$SRC/cjson/fuzzing/json.dict" ]; then
        cp "$SRC/cjson/fuzzing/json.dict" "$OUT/${fuzzer}.dict"
    fi
done