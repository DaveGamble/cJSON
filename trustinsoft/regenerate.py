#! /usr/bin/env python3

# This script regenerates TrustInSoft CI configuration.

# Run from the root of the cJSON project:
# $ python3 trustinsoft/regenerate.py

import re # sub
import json # dumps, load
from os import path # basename, isdir, join
import glob # iglob

# Outputting JSON.
def string_of_json(obj):
    # Output standard pretty-printed JSON (RFC 7159) with 4-space indentation.
    s = json.dumps(obj, indent=4)
    # Sometimes we need to have multiple "include" fields in the outputted
    # JSON, which is unfortunately impossible in the internal python
    # representation (OK, it is technically possible, but too cumbersome to
    # bother implementing it here), so we can name these fields 'include_',
    # 'include__', etc, and they are all converted to 'include' before
    # outputting as JSON.
    s = re.sub(r'"include_+"', '"include"', s)
    return s

# Make a command line from a dictionary of lists.
def string_of_options(options):
    elts = []
    for opt_prefix in options: # e.g. opt_prefix == "-D"
        for opt_value in options[opt_prefix]: # e.g. opt_value == "HAVE_OPEN"
            elts.append(opt_prefix + opt_value) # e.g. "-DHAVE_OPEN"
    return " ".join(elts)


# Directories.
test_files_dir = "tests"
fuzz_input_dir = path.join("fuzzing", "inputs")

# --------------------------------------------------------------------------- #
# ---------------------------------- CHECKS --------------------------------- #
# --------------------------------------------------------------------------- #

def check_dir(dir):
    if path.isdir(dir):
        print("   > OK! Directory '%s' exists." % dir)
    else:
        exit("Directory '%s' not found." % dir)

# Initial check.
print("1. Check if all necessary directories and files exist...")
check_dir("trustinsoft")
check_dir(test_files_dir)
check_dir(fuzz_input_dir)

# --------------------------------------------------------------------------- #
# -------------------- GENERATE trustinsoft/common.config ------------------- #
# --------------------------------------------------------------------------- #

common_config_path = path.join("trustinsoft", "common.config")

def make_common_config():
    # C files.
    c_files = [
        "cJSON_Utils.c",
        path.join("tests", "unity", "src", "unity.c"),
    ]
    # Compilation options.
    compilation_cmd = (
        {
            "-I": [],
            "-D": [
                "UNITY_EXCLUDE_SETJMP_H"
            ],
            "-U": [],
        }
    )
    # Filesystem.
    json_patch_tests = list(
        map(lambda file:
            {
                "name": path.join("json-patch-tests", path.basename(file)),
                "from": path.join("..", file),
            },
            sorted(glob.iglob(path.join("tests", "json-patch-tests", "*.json"),
                   recursive=False)))
    )
    tests_and_expected = list(
        map(lambda file:
            {
                "name": path.join("inputs", path.basename(file)),
                "from": path.join("..", file),
            },
            sorted(glob.iglob(path.join("tests", "inputs", "test*"),
                              recursive=False)))
    )
    # Whole common.config JSON.
    config = (
        {
            "files": list(map(lambda file: path.join("..", file), c_files)),
            "compilation_cmd": string_of_options(compilation_cmd),
            "val-clone-on-recursive-calls-max-depth": 10000,
            "filesystem": { "files": json_patch_tests + tests_and_expected },
        }
    )
    # Done.
    return config

common_config = make_common_config()
with open(common_config_path, "w") as file:
    print("2. Generate the '%s' file." % common_config_path)
    file.write(string_of_json(common_config))

# --------------------------------------------------------------------------- #
# -------------------- GENERATE trustinsoft/fuzz.config --------------------- #
# --------------------------------------------------------------------------- #

fuzz_config_path = path.join("trustinsoft", "fuzz.config")

def make_fuzz_config():
    # C files.
    c_files = [
        "cJSON.c",
        path.join("fuzzing", "afl.c"),
    ]
    # Filesystem.
    fuzzing_files = list(
        map(lambda file:
            {
                "name": path.join(fuzz_input_dir, path.basename(file)),
                "from": path.join("..", file),
            },
            sorted(glob.iglob(path.join(fuzz_input_dir, "test*"),
                              recursive=False)))
    )
    # Whole fuzz.config JSON.
    config = (
        {
            "files": list(map(lambda file: path.join("..", file), c_files)),
            "filesystem": { "files": fuzzing_files },
        }
    )
    # Done.
    return config

fuzz_config = make_fuzz_config()
with open(fuzz_config_path, "w") as file:
    print("3. Generate the '%s' file." % fuzz_config_path)
    file.write(string_of_json(fuzz_config))

# --------------------------------------------------------------------------- #
# -------------------------------- tis.config ------------------------------- #
# --------------------------------------------------------------------------- #

exclude_tests = [
    "unity_setup.c"
]

def test_files():
    test_files = sorted(
        glob.iglob(path.join(test_files_dir, "*.c"), recursive=False)
    )
    for exclude_test in exclude_tests:
        test_files.remove(path.join("tests", exclude_test))
    return test_files

def make_test(test_file):
    basename = path.basename(test_file)
    return (
        {
            "name": basename,
            "files": [ test_file ],
            "include": common_config_path,
        }
    )

def fuzz_input_files():
    return sorted(
        glob.iglob(path.join(fuzz_input_dir, "test*"), recursive=False)
    )

def make_fuzz_test(fuzz_input_file):
    basename = path.basename(fuzz_input_file)
    return (
        {
            "name": ("afl.c " + basename),
            "val-args": " " + path.join(fuzz_input_dir, basename),
            "include": common_config_path,
            "include_": fuzz_config_path,
        }
    )

tis_config = (
    list(map(make_test, test_files())) +
    list(map(make_fuzz_test, fuzz_input_files()))
)
with open("tis.config", "w") as file:
    print("4. Generate the 'tis.config' file.")
    file.write(string_of_json(tis_config))
