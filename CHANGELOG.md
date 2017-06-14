1.5.5
=====
Fixes:
------
* Fix pointers to nested arrays in cJSON_Utils (9abe75e072050f34732a7169740989a082b65134)
* Fix an error with case sensitivity handling in cJSON_Utils (b9cc911831b0b3e1bb72f142389428e59f882b38)
* Fix cJSON_Compare for arrays that are prefixes of the other and objects that are a subset of the other (03ba72faec115160d1f3aea5582d9b6af5d3e473) See #180, thanks @zhengqb for reporting

1.5.4
=====
Fixes:
------
* Fix build with GCC 7.1.1 and optimization level `-O2` (bfbd8fe0d85f1dd21e508748fc10fc4c27cc51be)

Other Changes:
--------------
* Update [Unity](https://github.com/ThrowTheSwitch/Unity) to 3b69beaa58efc41bbbef70a32a46893cae02719d

1.5.3
=====
Fixes:
------
* Fix `cJSON_ReplaceItemInObject` not keeping the name of an item (#174)

1.5.2
=====
Fixes:
------
* Fix a reading buffer overflow in `parse_string` (a167d9e381e5c84bc03de4e261757b031c0c690d)
* Fix compiling with -Wcomma (186cce3ece6ce6dfcb58ac8b2a63f7846c3493ad)
* Remove leftover attribute from tests (b537ca70a35680db66f1f5b8b437f7114daa699a)

1.5.1
=====
Fixes:
------
* Add gcc version guard to the Makefile (#164), thanks @juvasquezg
* Fix incorrect free in `cJSON_Utils` if custom memory allocator is used (#166), thanks @prefetchnta

1.5.0
=====
Features:
---------
* cJSON finally prints numbers without losing precision (#153) thanks @DeboraG
* `cJSON_Compare` recursively checks if two cJSON items contain the same values (#148)
* Provide case sensitive versions of every function where it matters (#158, #159)
* Added `cJSON_ReplaceItemViaPointer` and `cJSON_DetachItemViaPointer`
* Added `cJSON_free` and `cJSON_malloc` that expose the internal configured memory allocators. (02a05eea4e6ba41811f130b322660bea8918e1a0)


Enhancements:
-------------
* Parse into a buffer, this will allow parsing `\u0000` in the future (not quite yet though)
* General simplifications and readability improvements
* More unit tests
* Update [unity](https://github.com/ThrowTheSwitch/Unity) testing library to 2.4.1
* Add the [json-patch-tests](https://github.com/json-patch/json-patch-tests) test suite to test cJSON_Utils.
* Move all tests from `test_utils.c` to unit tests with unity.

Fixes:
------
* Fix some warnings with the Microsoft compiler (#139) thanks @PawelWMS
* Fix several bugs in cJSON_Utils, mostly found with [json-patch-tests](https://github.com/json-patch/json-patch-tests)
* Prevent a stack overflow by specifying a maximum nesting depth `CJSON_NESTING_LIMIT`

Other Changes:
--------------
* Move generated files in the `library_config` subdirectory.

1.4.7
=====
Fixes:
------
* Fix `cJSONUtils_ApplyPatches`, it was completely broken and apparently nobody noticed (or at least reported it) (075a06f40bdc4f836c7dd7cad690d253a57cfc50)
* Fix inconsistent prototype for `cJSON_GetObjectItemCaseSensitive` (51d3df6c9f7b56b860c8fb24abe7bab255cd4fa9) thanks @PawelWMS

1.4.6
=====
Fixes:
------
* Several corrections in the README
* Making clear that `valueint` should not be written to
* Fix overflow detection in `ensure` (2683d4d9873df87c4bdccc523903ddd78d1ad250)
* Fix a potential null pointer dereference in cJSON_Utils (795c3acabed25c9672006b2c0f40be8845064827)
* Replace incorrect `sizeof('\0')` with `sizeof("")` (84237ff48e69825c94261c624eb0376d0c328139)
* Add caveats section to the README (50b3c30dfa89830f8f477ce33713500740ac3b79)
* Make cJSON locale independent (#146) Thanks @peterh for reporting
* Fix compiling without CMake with MSVC (#147) Thanks @dertuxmalwieder for reporting

1.4.5
=====
Fixes:
------
* Fix bug in `cJSON_SetNumberHelper`, thanks @mmkeeper (#138 ef34500693e8c4a2849d41a4bd66fd19c9ec46c2)
* Workaround for internal compiler error in GCC 5.4.0 and 6.3.1 on x86 (2f65e80a3471d053fdc3f8aed23d01dd1782a5cb [GCC bugreport](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80097))

1.4.4
=====
Fixes:
--------
* Fix a theoretical integer overflow, (not sure if it is possible on actual hardware) e58f7ec027d00b7cdcbf63e518c1b5268b29b3da
* Fix an off by one error (cc84a446be20cc283bafdc4d94c050ba1111ac02), thanks @gatzka
* Double check the offset of the print buffer in `ensure` (1934059554b9a0971e00f79e96900f422cfdd114)

Improvements:
-------------
* Add a note in the header about required buffer size when using `cJSON_PrintPreallocated` (4bfb88009342fb568295a7f6dc4b7fee74fbf022)

1.4.3
=====
Fixes:
------
* Fix compilation of the tests on 32 bit PowerPC and potentially other systems (4ec6e76ea2eec16f54b58e8c95b4c734e59481e4)
* Fix compilation with old GCC compilers (4.3+ were tested) (227d3398d6b967879761ebe02c1b63dbd6ea6e0d, 466eb8e3f8a65080f2b3ca4a79ab7b72bd539dba), see also #126

1.4.2
=====
Fixes:
------
* Fix minimum required cmake version (30e1e7af7c63db9b55f5a3cda977a6c032f0b132)
* Fix detection of supported compiler flags (76e5296d0d05ceb3018a9901639e0e171b44a557)
* Run `cJSON_test` and `cJSON_test_utils` along with unity tests (c597601cf151a757dcf800548f18034d4ddfe2cb)

1.4.1
=====
Fix: Make `print_number` abort with a failure in out of memory situations (cf1842dc6f64c49451a022308b4415e4d468be0a)

1.4.0
=====
Features
--------
* Functions to check the type of an item (#120)
* Use dllexport on windows and fvisibility on Unix systems for public functions (#116), thanks @mjerris
* Remove trailing zeroes from printed numbers (#123)
* Expose the internal boolean type `cJSON_bool` in the header (2d3520e0b9d0eb870e8886e8a21c571eeddbb310)

Fixes
-----
* Fix handling of NULL pointers in `cJSON_ArrayForEach` (b47d0e34caaef298edfb7bd09a72cfff21d231ff)
* Make it compile with GCC 7 (fix -Wimplicit-fallthrough warning) (9d07917feb1b613544a7513d19233d4c851ad7ad)

Other Improvements
------------------
* internally use realloc if available (#110)
* builtin support for fuzzing with [afl](http://lcamtuf.coredump.cx/afl/) (#111)
* unit tests for the print functions (#112)
* Always use buffered printing (#113)
* simplify the print functions (#114)
* Add the compiler flags `-Wdouble-conversion`, `-Wparentheses` and `-Wcomma` (#122)

1.3.2
=====
Fix:
----
- Don't build the unity library if testing is disabled ( #121 ). Thanks @ffontaine

1.3.1
=====
Bugfix release that fixes an out of bounds read #118. This shouldn't have any security implications.

1.3.0
=====
This release includes a lot of rework in the parser and includes the Cunity unit testing framework, as well as some fixes. I increased the minor version number because there were quite a lot of internal changes.

Features:
---------
- New type for cJSON structs: `cJSON_Invalid` (#108)

Fixes:
------
- runtime checks for a lot of potential integer overflows
- fix incorrect return in cJSON_PrintBuffered (cf9d57d56cac21fc59465b8d26cf29bf6d2a87b3)
- fix several potential issues found by [Coverity](https://scan.coverity.com/projects/cjson)
- fix potentially undefined behavior when assigning big numbers to `valueint` (41e2837df1b1091643aff073f2313f6ff3cc10f4)
  - Numbers exceeding `INT_MAX` or lower than `INT_MIN` will be explicitly assigned to `valueint` as `INT_MAX` and `INT_MIN` respectively (saturation on overflow).
  - fix the `cJSON_SetNumberValue` macro (87f77274de6b3af00fb9b9a7f3b900ef382296c2), this slightly changes the behavior, see commit message

Introduce unit tests
--------------------

Started writing unit tests with the [Cunity](https://github.com/ThrowTheSwitch/Unity) testing framework. Currently this covers the parser functions.

Also:
- Support for running the tests with [Valgrind](http://valgrind.org)
- Support for compiling the tests with [AddressSanitizer](https://github.com/google/sanitizers) and [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html).
- `travis.yml` file for running unit tests on travis. (not enabled for the repository yet though #102)

Simplifications
---------------

After having unit tests for the parser function in place, I started refactoring the parser functions (as well as others) and making them easier to read and maintain.
- Use `strtod` from the standard library for parsing numbers (074766997246481dfc72bfa78f07898a2716473f)
- Use goto-fail in several parser functions (#100)
- Rewrite/restructure all of the parsing functions to be easier to understand and have less code paths doing the same as another. (#109)
- Simplify the buffer allocation strategy to always doubling the needed amount (9f6fa94c91a87b71e4c6868dbf2ce431a48517b0)
- Combined `cJSON_AddItemToObject` and `cJSON_AddItemToObjectCS` to one function (cf862d0fed7f9407e4b046d78d3d8050d2080d12)

Other changes
-------------
- Prevent the usage of incompatible C and header versions via preprocessor directive (123bb1af7bfae41d805337fef4b41045ef6c7d25)
- Let CMake automatically detect compiler flags
- Add new compiler flags (`-Wundef`, `-Wswitch-default`, `-Wconversion`, `-fstack-protector-strong`) (#98)
- Change internal sizes from `int` to `size_t` (ecd5678527a6bc422da694e5be9e9979878fe6a0)
- Change internal strings from `char*` to `unsigned char*` (28b9ba4334e0f7309e867e874a31f395c0ac2474)
- Add `const` in more places

1.2.1
=====
Fixes:
------
- Fixes a potential null pointer dereference in cJSON_Utils, discovered using clang's static analyzer by @bnason-nf (#96)

1.2.0
=====
Features:
---------
- Add a new type of cJSON item for raw JSON and support printing it. Thanks @loigu (#65, #90)

Fixes:
------
- Compiler warning if const is casted away, Thanks @gatzka (#83)
- Fix compile error with strict-overflow on PowerPC, (#85)
- Fix typo in the README, thanks @MicroJoe (#88)
- Add compile flag for compatibility with C++ compilers

1.1.0
=====
- Add a function `cJSON_PrintPreallocated` to print to a preallocated buffer, thanks @ChisholmKyle (#72)
- More compiler warnings when using Clang or GCC, thanks @gatzka (#75, #78)
- fixed a memory leak in `cJSON_Duplicate`, thanks @alperakcan (#81)
- fix the `ENABLE_CUSTOM_COMPILER_FLAGS` cmake option

1.0.2
=====
Rename internal boolean type, see #71.

1.0.1
=====
Small bugfix release.
- Fixes a bug with the use of the cJSON structs type in cJSON_Utils, see d47339e2740360e6e0994527d5e4752007480f3a
- improve code readability
- initialize all variables

1.0.0
=====
This is the first official versioned release of cJSON. It provides an API version for the shared library and improved Makefile and CMake build files.
