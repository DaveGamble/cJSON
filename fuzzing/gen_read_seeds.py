#!/usr/bin/env python3
"""Generate a structured seed corpus for fuzzing/cjson_read_fuzzer.c.

The libFuzzer harness expects: four flag characters ('0'/'1') selecting
minify / require_termination / formatted / buffered printing, then the
JSON document, then a trailing NUL byte. The inputs shipped in
fuzzing/inputs are formatted for fuzzing/afl.c and never pass those
checks, so this script constructs seeds in the correct frame.

Deterministic: emits one file per (document, flag-combo) pair into the
directory given as argv[1]. No external samples are used.
"""
import os
import sys

DOCUMENTS = [
    # plain shapes
    "{}",
    "[]",
    '{"a":1}',
    "[1,2,3]",
    '{"a":{"b":[1,{"c":"d"}]}}',
    '[{"x":[1,2,{"y":null}]},[3],{}]',
    # number shapes
    '{"pi":3.14159,"big":1e300,"tiny":1e-300,"neg":-0,"exp":1.5E+10}',
    "[0,-1,1e10,-1e-10,2.2250738585072011e-308,1e999,-1e999]",
    # literals
    "[true,false,null,true]",
    '{"t":true,"f":false,"n":null}',
    # string escapes
    '{"s":"a\\"b\\\\c\\nd\\te\\ff\\br"}',
    '{"u":"\\u0041\\uD83D\\uDE00\\u0000"}'.replace("\\u0000", "\\uFFFD"),
    '{"solidus":"a/b","tilde":"a~b"}',
    # whitespace variants
    '{  "a" : 1 ,\t"b" : [ 2 , 3 ] \n }',
    "\r\n [ 1 , 2 ] \t ",
    # duplicate keys
    '{"a":1,"a":2,"A":3}',
    # bigger collections
    "[" + ",".join(str(i) for i in range(64)) + "]",
    "{" + ",".join('"k%d":%d' % (i, i) for i in range(32)) + "}",
    # deep nesting inside the parser limit
    "[" * 200 + "1" + "]" * 200,
    # over the CJSON_NESTING_LIMIT (1000): parse must fail cleanly
    "[" * 1001 + "1" + "]" * 1001,
    # scalars
    "42",
    '"hi"',
    "true",
    "null",
    '""',
    # parse-error paths
    '{"a":}',
    "[1,2",
    '{"a":"unterminated}',
    '{"a":1,}',
    '{"a" 1}',
    "nul",
    "truex",
    "01",
    "+1",
    ".5",
    "1.",
    # trailing content interacts with require_termination
    '{"a":1}x',
    "[1][2]",
    '1 2',
]

# Representative flag combinations per document; the trailing-garbage
# documents get every combination so all require_termination paths run.
FLAG_COMBOS = ["0000", "1111", "1010", "0101"]
ALL_FLAG_COMBOS = ["".join("1" if (i >> bit) & 1 else "0" for bit in range(3, -1, -1))
                   for i in range(16)]
TRAILING_GARBAGE = {"{" + "}" + "x", "[1][2]", "1 2"}


def main() -> None:
    if len(sys.argv) != 2:
        print("usage: gen_read_seeds.py <output-directory>", file=sys.stderr)
        return 1

    out_dir = sys.argv[1]
    os.makedirs(out_dir, exist_ok=True)

    count = 0
    for doc_index, document in enumerate(DOCUMENTS):
        combos = ALL_FLAG_COMBOS if document in TRAILING_GARBAGE else FLAG_COMBOS
        for combo_index, flags in enumerate(combos):
            name = "seed_%02d_%s.json" % (doc_index, flags)
            payload = flags + document + "\0"
            with open(os.path.join(out_dir, name), "wb") as handle:
                handle.write(payload.encode("utf-8"))
            count += 1

    print("generated %d seeds in %s" % (count, out_dir))
    return 0


if __name__ == "__main__":
    sys.exit(main())