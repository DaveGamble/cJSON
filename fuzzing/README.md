# Additional cJSON OSS-Fuzz Harnesses

This directory contains additional fuzz targets for cJSON.

The goal is to test more public cJSON APIs, not only JSON parsing. These
harnesses build small cJSON objects, run API operations on them, and check simple
expected behavior.

The harnesses are meant to help OSS-Fuzz find:

- memory safety bugs,
- undefined behavior,
- incorrect API behavior,
- ownership mistakes,
- buffer boundary errors.

They do not claim to fully verify cJSON.

## Added Fuzz Targets

| Fuzzer | What it tests |
| --- | --- |
| `cjson_create_fuzzer.c` | cJSON create APIs, array helpers, string/raw creation, and reference creation |
| `cjson_add_to_object_fuzzer.c` | Adding numbers, strings, arrays, objects, raw values, and references to objects |
| `cjson_delete_item_from_object_fuzzer.c` | Deleting and detaching items from objects and arrays |
| `cjson_replace_item_in_object_fuzzer.c` | Replacing object items with case-sensitive and case-insensitive lookup |
| `cjson_duplicate_fuzzer.c` | Recursive and non-recursive duplication |
| `cjson_compare_fuzzer.c` | Strict and loose comparison behavior |
| `cjson_print_preallocated_fuzzer.c` | Printing into caller-provided buffers |

## What the Harnesses Check

The harnesses use simple API-level checks. Examples include:

- successful add operations should make the key reachable,
- failed add operations should not change the object size,
- deleting an existing item should remove it,
- deleting a missing item should leave the object unchanged,
- replacing an existing key should succeed without changing object size,
- replacing a missing key should fail,
- duplicated trees should not share mutable nodes with the original,
- comparison should be reflexive and symmetric,
- preallocated printing should match normal cJSON printing,
- preallocated printing should not write past the provided buffer,
- invalid inputs such as `NULL` pointers should fail cleanly where expected.

The harnesses also use sanitizers through OSS-Fuzz, so memory errors and
undefined behavior can still be reported normally.

## Design Notes

Most targets use small seeded cJSON trees. This makes it easier for the fuzzer
to reach useful API states, such as existing keys, nested objects, arrays, and
replacement targets.

The fuzz input controls operation choices, keys, values, case sensitivity,
array indices, and buffer sizes. All loops, string sizes, array sizes, and
generated subtrees are bounded so that the fuzzers remain suitable for
continuous fuzzing.

Some harnesses also parse part of the fuzz input as JSON after running their
structured tests. This keeps arbitrary parser-generated trees in scope while
still adding stronger API-specific checks.

## Round-Trip Checks

Some cJSON values are valid but are not stable under print/parse comparison.
Because of that, the harnesses avoid strict round-trip equality checks for:

- raw nodes,
- `NaN` and infinity,
- ambiguous duplicate object keys.

Those cases are still tested, but the harnesses use narrower checks such as
printability, exact output comparison, object size changes, or key reachability.

## Ownership

The harnesses follow cJSON ownership rules carefully.

In general:

- if an item is successfully added to a tree, the tree owns it,
- if an add or replace operation fails, the harness deletes the unused item,
- if an item is detached, the harness owns it and deletes it once,
- reference APIs are tested separately because they intentionally alias caller
  storage or child pointers.

This avoids false positives caused by bugs in the fuzz harness itself.

## Project Results

In our project evaluation, these harnesses increased runtime coverage of
`cJSON.c`:

| Metric | Before | After |
| --- | ---: | ---: |
| Line coverage | 42.93% | 80.32% |
| Function coverage | 27.43% | 92.04% |
| Region coverage | 44.89% | 84.61% |

Fuzz Introspector also reported:

| Metric | Result |
| --- | ---: |
| Statically reachable functions | 236 / 245 |
| Statically reachable cyclomatic complexity | 1337 / 1366 |
| Runtime-covered functions | 218 / 245 |

No confirmed ASan or UBSan crash was found during our project runs.

These numbers are included as project evaluation results. They may differ from
future OSS-Fuzz production runs depending on corpus state, build settings,
fuzzing time, and sanitizer configuration.

## Running Locally

From an OSS-Fuzz checkout:

```sh
python3 infra/helper.py build_image cjson
python3 infra/helper.py build_fuzzers --sanitizer address cjson
python3 infra/helper.py check_build cjson