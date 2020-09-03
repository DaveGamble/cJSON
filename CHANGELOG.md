1.7.14 (Sep 3, 2020)
======
Fixes:
------
* optimize the way to find tail node, see [#503](https://github.com/DaveGamble/cJSON/pull/503)
* Fix WError error on macosx because NAN is a float. Thanks @sappo, see [#484](https://github.com/DaveGamble/cJSON/pull/484)
* Fix some bugs in detach and replace. Thanks @miaoerduo, see [#456](https://github.com/DaveGamble/cJSON/pull/456)

1.7.13 (Apr 2, 2020)
======
Features:
---------
* add new API of cJSON_ParseWithLength without breaking changes. Thanks @caglarivriz, see [#358](https://github.com/DaveGamble/cJSON/pull/358)  
* add new API of cJSON_GetNumberValue. Thanks @Intuition, see[#385](https://github.com/DaveGamble/cJSON/pull/385)  
* add uninstall target function for CMake. See [#402](https://github.com/DaveGamble/cJSON/pull/402)  
* Improve performance of adding item to array. Thanks @xiaomianhehe, see [#430](https://github.com/DaveGamble/cJSON/pull/430), [#448](https://github.com/DaveGamble/cJSON/pull/448)  
* add new API of cJSON_SetValuestring, for changing the valuestring safely. See [#451](https://github.com/DaveGamble/cJSON/pull/451)  
* add return value for cJSON_AddItemTo... and cJSON_ReplaceItem... (check if the operation successful). See [#453](https://github.com/DaveGamble/cJSON/pull/453)  

Fixes:
------
* Fix clang -Wfloat-equal warning. Thanks @paulmalovanyi, see [#368](https://github.com/DaveGamble/cJSON/pull/368)  
* Fix make failed in mac os. See [#405](https://github.com/DaveGamble/cJSON/pull/405)  
* Fix memory leak in cJSONUtils_FindPointerFromObjectTo. Thanks @andywolk for reporting, see [#414](https://github.com/DaveGamble/cJSON/issues/414)  
* Fix bug in encode_string_as_pointer. Thanks @AIChangJiang for reporting, see [#439](https://github.com/DaveGamble/cJSON/issues/439)  

1.7.12 (May 17, 2019)
======
Fixes:
------
* Fix infinite loop in `cJSON_Minify` (potential Denial of Service). Thanks @Alanscut for reporting, see [#354](https://github.com/DaveGamble/cJSON/issues/354)
* Fix link error for Visual Studio. Thanks @tan-wei, see [#352](https://github.com/DaveGamble/cJSON/pull/352).
* Undefine `true` and `false` for `cJSON_Utils` before redefining them. Thanks @raiden00pl, see [#347](https://github.com/DaveGamble/cJSON/pull/347).

1.7.11 (Apr 15, 2019)
======
Fixes:
------
* Fix a bug where cJSON_Minify could overflow it's buffer, both reading and writing. This is a security issue, see [#338](https://github.com/DaveGamble/cJSON/issues/338). Big thanks @bigric3 for reporting.
* Unset `true` and `false` macros before setting them if they exist. See [#339](https://github.com/DaveGamble/cJSON/issues/339), thanks @raiden00pl for reporting

1.7.10 (Dec 21, 2018)
======
Fixes:
------
* Fix package config file for `libcjson`. Thanks @shiluotang for reporting [#321](https://github.com/DaveGamble/cJSON/issues/321)
* Correctly split lists in `cJSON_Utils`'s merge sort. Thanks @andysCaplin for the fix [#322](https://github.com/DaveGamble/cJSON/issues/322)

1.7.9 (Dec 16, 2018)
=====
Fixes:
------
* Fix a bug where `cJSON_GetObjectItemCaseSensitive` would pass a nullpointer to `strcmp` when called on an array, see [#315](https://github.com/DaveGamble/cJSON/issues/315). Thanks @yuweol for reporting.
* Fix error in `cJSON_Utils` where the case sensitivity was not respected, see [#317](https://github.com/DaveGamble/cJSON/pull/317). Thanks @yuta-oxo for fixing.
* Fix some warnings detected by the Visual Studio Static Analyzer, see [#307](https://github.com/DaveGamble/cJSON/pull/307). Thanks @bnason-nf

1.7.8 (Sep 22, 2018)
======
Fixes:
------
* cJSON now works with the `__stdcall` calling convention on Windows, see [#295](https://github.com/DaveGamble/cJSON/pull/295), thanks @zhindes for contributing

1.7.7 (May 22, 2018)
=====
Fixes:
------
* Fix a memory leak when realloc fails, see [#267](https://github.com/DaveGamble/cJSON/issues/267), thanks @AlfieDeng for reporting
* Fix a typo in the header file, see [#266](https://github.com/DaveGamble/cJSON/pull/266), thanks @zhaozhixu

1.7.6 (Apr 13, 2018)
=====
Fixes:
------
* Add `SONAME` to the ELF files built by the Makefile, see [#252](https://github.com/DaveGamble/cJSON/issues/252), thanks @YanhaoMo for reporting
* Add include guards and `extern "C"` to `cJSON_Utils.h`, see [#256](https://github.com/DaveGamble/cJSON/issues/256), thanks @daschfg for reporting

Other changes:
* Mark the Makefile as deprecated in the README.

1.7.5 (Mar 23, 2018)
=====
Fixes:
------
* Fix a bug in the JSON Patch implementation of `cJSON Utils`, see [#251](https://github.com/DaveGamble/cJSON/pull/251), thanks @bobkocisko.

1.7.4 (Mar 3, 2018)
=====
Fixes:
------
* Fix potential use after free if the `string` parameter to `cJSON_AddItemToObject` is an alias of the `string` property of the object that is added,see [#248](https://github.com/DaveGamble/cJSON/issues/248). Thanks @hhallen for reporting.

1.7.3 (Feb 8, 2018)
=====
Fixes:
------
* Fix potential double free, thanks @projectgus for reporting [#241](https://github.com/DaveGamble/cJSON/issues/241)

1.7.2 (Feb 6, 2018)
=====
Fixes:
------
* Fix the use of GNUInstallDirs variables and the pkgconfig file. Thanks @zeerd for reporting [#240](https://github.com/DaveGamble/cJSON/pull/240)

1.7.1 (Jan 10, 2018)
=====
Fixes:
------
* Fixed an Off-By-One error that could lead to an out of bounds write. Thanks @liuyunbin for reporting [#230](https://github.com/DaveGamble/cJSON/issues/230)
* Fixed two errors with buffered printing. Thanks @liuyunbin for reporting [#230](https://github.com/DaveGamble/cJSON/issues/230)

1.7.0 (Dec 31, 2017)
=====
Features:
---------
* Large rewrite of the documentation, see [#215](https://github.com/DaveGamble/cJSON/pull/215)
* Added the `cJSON_GetStringValue` function
* Added the `cJSON_CreateStringReference` function
* Added the `cJSON_CreateArrayReference` function
* Added the `cJSON_CreateObjectReference` function
* The `cJSON_Add...ToObject` macros are now functions that return a pointer to the added item, see [#226](https://github.com/DaveGamble/cJSON/pull/226)

Fixes:
------
* Fix a problem with `GNUInstallDirs` in the CMakeLists.txt, thanks @yangfl, see [#210](https://github.com/DaveGamble/cJSON/pull/210)
* Fix linking the tests when building as static library, see [#213](https://github.com/DaveGamble/cJSON/issues/213)
* New overrides for the CMake option `BUILD_SHARED_LIBS`, see [#207](https://github.com/DaveGamble/cJSON/issues/207)

Other Changes:
--------------
* Readme: Explain how to include cJSON, see [#211](https://github.com/DaveGamble/cJSON/pull/211)
* Removed some trailing spaces in the code, thanks @yangfl, see [#212](https://github.com/DaveGamble/cJSON/pull/212)
* Updated [Unity](https://github.com/ThrowTheSwitch/Unity) and [json-patch-tests](https://github.com/json-patch/json-patch-tests)

1.6.0 (Oct 9, 2017)
=====
Features:
---------
* You can now build cJSON as both shared and static library at once with CMake using `-DBUILD_SHARED_AND_STATIC_LIBS=On`, see [#178](https://github.com/DaveGamble/cJSON/issues/178)
* UTF-8 byte order marks are now ignored, see [#184](https://github.com/DaveGamble/cJSON/issues/184)
* Locales can now be disabled with the option `-DENABLE_LOCALES=Off`, see [#202](https://github.com/DaveGamble/cJSON/issues/202), thanks @Casperinous
* Better support for MSVC and Visual Studio

Other Changes:
--------------
* Add the new warnings `-Wswitch-enum`, `-Wused-but-makred-unused`, `-Wmissing-variable-declarations`, `-Wunused-macro`
* More number printing tests.
* Continuous integration testing with AppVeyor (semi automatic at this point), thanks @simon-p-r

1.5.9 (Sep 8, 2017)
=====
Fixes:
------
* Set the global error pointer even if `return_parse_end` is passed to `cJSON_ParseWithOpts`, see [#200](https://github.com/DaveGamble/cJSON/pull/200), thanks @rmallins

1.5.8 (Aug 21, 2017)
=====
Fixes:
------
* Fix `make test` in the Makefile, thanks @YanhaoMo for reporting this [#195](https://github.com/DaveGamble/cJSON/issues/195)

1.5.7 (Jul 13, 2017)
=====
Fixes:
------
* Fix a bug where realloc failing would return a pointer to an invalid memory address. This is a security issue as it could potentially be used by an attacker to write to arbitrary memory addresses, see [#189](https://github.com/DaveGamble/cJSON/issues/189),  fixed in [954d61e](https://github.com/DaveGamble/cJSON/commit/954d61e5e7cb9dc6c480fc28ac1cdceca07dd5bd), big thanks @timothyjohncarney for reporting this issue
* Fix a spelling mistake in the AFL fuzzer dictionary, see [#185](https://github.com/DaveGamble/cJSON/pull/185), thanks @jwilk

1.5.6 (Jun 28, 2017)
=====
Fixes:
------
* Make cJSON a lot more tolerant about passing NULL pointers to its functions, it should now fail safely instead of dereferencing the pointer, see [#183](https://github.com/DaveGamble/cJSON/pull/183). Thanks @msichal for reporting [#182](https://github.com/DaveGamble/cJSON/issues/182)

1.5.5 (Jun 15, 2017)
=====
Fixes:
------
* Fix pointers to nested arrays in cJSON_Utils, see [9abe](https://github.com/DaveGamble/cJSON/commit/9abe75e072050f34732a7169740989a082b65134)
* Fix an error with case sensitivity handling in cJSON_Utils, see [b9cc911](https://github.com/DaveGamble/cJSON/commit/b9cc911831b0b3e1bb72f142389428e59f882b38)
* Fix cJSON_Compare for arrays that are prefixes of the other and objects that are a subset of the other, see [03ba72f](https://github.com/DaveGamble/cJSON/commit/03ba72faec115160d1f3aea5582d9b6af5d3e473) and [#180](https://github.com/DaveGamble/cJSON/issues/180), thanks @zhengqb for reporting

1.5.4 (Jun 5, 2017)
======
Fixes:
------
* Fix build with GCC 7.1.1 and optimization level `-O2`, see [bfbd8fe](https://github.com/DaveGamble/cJSON/commit/bfbd8fe0d85f1dd21e508748fc10fc4c27cc51be)

Other Changes:
--------------
* Update [Unity](https://github.com/ThrowTheSwitch/Unity) to 3b69beaa58efc41bbbef70a32a46893cae02719d

1.5.3 (May 23, 2017)
=====
Fixes:
------
* Fix `cJSON_ReplaceItemInObject` not keeping the name of an item, see [#174](https://github.com/DaveGamble/cJSON/issues/174)

1.5.2 (May 10, 2017)
=====
Fixes:
------
* Fix a reading buffer overflow in `parse_string`, see [a167d9e](https://github.com/DaveGamble/cJSON/commit/a167d9e381e5c84bc03de4e261757b031c0c690d)
* Fix compiling with -Wcomma, see [186cce3](https://github.com/DaveGamble/cJSON/commit/186cce3ece6ce6dfcb58ac8b2a63f7846c3493ad)
* Remove leftover attribute from tests, see [b537ca7](https://github.com/DaveGamble/cJSON/commit/b537ca70a35680db66f1f5b8b437f7114daa699a)

1.5.1 (May 6, 2017)
=====
Fixes:
------
* Add gcc version guard to the Makefile, see [#164](https://github.com/DaveGamble/cJSON/pull/164), thanks @juvasquezg
* Fix incorrect free in `cJSON_Utils` if custom memory allocator is used, see [#166](https://github.com/DaveGamble/cJSON/pull/166), thanks @prefetchnta

1.5.0 (May 2, 2017)
=====
Features:
* cJSON finally prints numbers without losing precision, see [#153](https://github.com/DaveGamble/cJSON/pull/153), thanks @DeboraG
* `cJSON_Compare` recursively checks if two cJSON items contain the same values, see [#148](https://github.com/DaveGamble/cJSON/pull/148)
* Provide case sensitive versions of every function where it matters, see [#158](https://github.com/DaveGamble/cJSON/pull/158) and [#159](https://github.com/DaveGamble/cJSON/pull/159)
* Added `cJSON_ReplaceItemViaPointer` and `cJSON_DetachItemViaPointer`
* Added `cJSON_free` and `cJSON_malloc` that expose the internal configured memory allocators. see [02a05ee](https://github.com/DaveGamble/cJSON/commit/02a05eea4e6ba41811f130b322660bea8918e1a0)


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
* Fix some warnings with the Microsoft compiler, see [#139](https://github.com/DaveGamble/cJSON/pull/139), thanks @PawelWMS
* Fix several bugs in cJSON_Utils, mostly found with [json-patch-tests](https://github.com/json-patch/json-patch-tests)
* Prevent a stack overflow by specifying a maximum nesting depth `CJSON_NESTING_LIMIT`

Other Changes:
--------------
* Move generated files in the `library_config` subdirectory.

1.4.7 (Apr 19, 2017)
=====
Fixes:
------
* Fix `cJSONUtils_ApplyPatches`, it was completely broken and apparently nobody noticed (or at least reported it), see [075a06f](https://github.com/DaveGamble/cJSON/commit/075a06f40bdc4f836c7dd7cad690d253a57cfc50)
* Fix inconsistent prototype for `cJSON_GetObjectItemCaseSensitive`, see [51d3df6](https://github.com/DaveGamble/cJSON/commit/51d3df6c9f7b56b860c8fb24abe7bab255cd4fa9), thanks @PawelWMS

1.4.6 (Apr 9, 2017)
=====
Fixes:
------
* Several corrections in the README
* Making clear that `valueint` should not be written to
* Fix overflow detection in `ensure`, see [2683d4d](https://github.com/DaveGamble/cJSON/commit/2683d4d9873df87c4bdccc523903ddd78d1ad250)
* Fix a potential null pointer dereference in cJSON_Utils, see [795c3ac](https://github.com/DaveGamble/cJSON/commit/795c3acabed25c9672006b2c0f40be8845064827)
* Replace incorrect `sizeof('\0')` with `sizeof("")`, see [84237ff](https://github.com/DaveGamble/cJSON/commit/84237ff48e69825c94261c624eb0376d0c328139)
* Add caveats section to the README, see [50b3c30](https://github.com/DaveGamble/cJSON/commit/50b3c30dfa89830f8f477ce33713500740ac3b79)
* Make cJSON locale independent, see [#146](https://github.com/DaveGamble/cJSON/pull/146), Thanks @peterh for reporting
* Fix compiling without CMake with MSVC, see [#147](https://github.com/DaveGamble/cJSON/pull/147), Thanks @dertuxmalwieder for reporting

1.4.5 (Mar 28, 2017)
=====
Fixes:
------
* Fix bug in `cJSON_SetNumberHelper`, thanks @mmkeeper, see [#138](https://github.com/DaveGamble/cJSON/issues/138) and [ef34500](https://github.com/DaveGamble/cJSON/commit/ef34500693e8c4a2849d41a4bd66fd19c9ec46c2)
* Workaround for internal compiler error in GCC 5.4.0 and 6.3.1 on x86 (2f65e80a3471d053fdc3f8aed23d01dd1782a5cb [GCC bugreport](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80097))

1.4.4 (Mar 24, 2017)
=====
Fixes:
------
* Fix a theoretical integer overflow, (not sure if it is possible on actual hardware), see [e58f7ec](https://github.com/DaveGamble/cJSON/commit/e58f7ec027d00b7cdcbf63e518c1b5268b29b3da)
* Fix an off by one error, see [cc84a44](https://github.com/DaveGamble/cJSON/commit/cc84a446be20cc283bafdc4d94c050ba1111ac02), thanks @gatzka
* Double check the offset of the print buffer in `ensure`, see [1934059](https://github.com/DaveGamble/cJSON/commit/1934059554b9a0971e00f79e96900f422cfdd114)

Improvements:
* Add a note in the header about required buffer size when using `cJSON_PrintPreallocated`, see [4bfb8800](https://github.com/DaveGamble/cJSON/commit/4bfb88009342fb568295a7f6dc4b7fee74fbf022)

1.4.3 (Mar 19, 2017)
=====
Fixes:
------
* Fix compilation of the tests on 32 bit PowerPC and potentially other systems, see [4ec6e76](https://github.com/DaveGamble/cJSON/commit/4ec6e76ea2eec16f54b58e8c95b4c734e59481e4)
* Fix compilation with old GCC compilers (4.3+ were tested), see [227d33](https://github.com/DaveGamble/cJSON/commit/227d3398d6b967879761ebe02c1b63dbd6ea6e0d), [466eb8e](https://github.com/DaveGamble/cJSON/commit/466eb8e3f8a65080f2b3ca4a79ab7b72bd539dba), see also [#126](https://github.com/DaveGamble/cJSON/issues/126)

1.4.2 (Mar 16, 2017)
=====
Fixes:
------
* Fix minimum required cmake version, see [30e1e7a](https://github.com/DaveGamble/cJSON/commit/30e1e7af7c63db9b55f5a3cda977a6c032f0b132)
* Fix detection of supported compiler flags, see [76e5296](https://github.com/DaveGamble/cJSON/commit/76e5296d0d05ceb3018a9901639e0e171b44a557)
* Run `cJSON_test` and `cJSON_test_utils` along with unity tests, see [c597601](https://github.com/DaveGamble/cJSON/commit/c597601cf151a757dcf800548f18034d4ddfe2cb)

1.4.1 (Mar 16, 2017)
=====
Fixes:
------ 
* Make `print_number` abort with a failure in out of memory situations, see [cf1842](https://github.com/DaveGamble/cJSON/commit/cf1842dc6f64c49451a022308b4415e4d468be0a)

1.4.0 (Mar 4, 2017)
=====
Features
--------
* Functions to check the type of an item, see [#120](https://github.com/DaveGamble/cJSON/pull/120)
* Use dllexport on windows and fvisibility on Unix systems for public functions, see [#116](https://github.com/DaveGamble/cJSON/pull/116), thanks @mjerris
* Remove trailing zeroes from printed numbers, see [#123](https://github.com/DaveGamble/cJSON/pull/123)
* Expose the internal boolean type `cJSON_bool` in the header, see [2d3520e](https://github.com/DaveGamble/cJSON/commit/2d3520e0b9d0eb870e8886e8a21c571eeddbb310)

Fixes
* Fix handling of NULL pointers in `cJSON_ArrayForEach`, see [b47d0e3](https://github.com/DaveGamble/cJSON/commit/b47d0e34caaef298edfb7bd09a72cfff21d231ff)
* Make it compile with GCC 7 (fix -Wimplicit-fallthrough warning), see [9d07917](https://github.com/DaveGamble/cJSON/commit/9d07917feb1b613544a7513d19233d4c851ad7ad)

Other Improvements
* internally use realloc if available ([#110](https://github.com/DaveGamble/cJSON/pull/110))
* builtin support for fuzzing with [afl](http://lcamtuf.coredump.cx/afl/) ([#111](https://github.com/DaveGamble/cJSON/pull/111))
* unit tests for the print functions ([#112](https://github.com/DaveGamble/cJSON/pull/112))
* Always use buffered printing ([#113](https://github.com/DaveGamble/cJSON/pull/113))
* simplify the print functions ([#114](https://github.com/DaveGamble/cJSON/pull/114))
* Add the compiler flags `-Wdouble-conversion`, `-Wparentheses` and `-Wcomma` ([#122](https://github.com/DaveGamble/cJSON/pull/122))

1.3.2 (Mar 1, 2017)
=====
Fixes:
------
* Don't build the unity library if testing is disabled, see [#121](https://github.com/DaveGamble/cJSON/pull/121). Thanks @ffontaine

1.3.1 (Feb 27, 2017)
=====
Fixes:
------
* Bugfix release that fixes an out of bounds read, see [#118](https://github.com/DaveGamble/cJSON/pull/118). This shouldn't have any security implications.

1.3.0 (Feb 17, 2017)
=====
This release includes a lot of rework in the parser and includes the Cunity unit testing framework, as well as some fixes. I increased the minor version number because there were quite a lot of internal changes.

Features:
* New type for cJSON structs: `cJSON_Invalid`, see [#108](https://github.com/DaveGamble/cJSON/pull/108)

Fixes:
------
* runtime checks for a lot of potential integer overflows
* fix incorrect return in cJSON_PrintBuffered [cf9d57d](https://github.com/DaveGamble/cJSON/commit/cf9d57d56cac21fc59465b8d26cf29bf6d2a87b3)
* fix several potential issues found by [Coverity](https://scan.coverity.com/projects/cjson)
* fix potentially undefined behavior when assigning big numbers to `valueint` ([41e2837](https://github.com/DaveGamble/cJSON/commit/41e2837df1b1091643aff073f2313f6ff3cc10f4))
  * Numbers exceeding `INT_MAX` or lower than `INT_MIN` will be explicitly assigned to `valueint` as `INT_MAX` and `INT_MIN` respectively (saturation on overflow).
  * fix the `cJSON_SetNumberValue` macro ([87f7727](https://github.com/DaveGamble/cJSON/commit/87f77274de6b3af00fb9b9a7f3b900ef382296c2)), this slightly changes the behavior, see commit message

Introduce unit tests
--------------------

* Started writing unit tests with the [Cunity](https://github.com/ThrowTheSwitch/Unity) testing framework. Currently this covers the parser functions.

Also:
* Support for running the tests with [Valgrind](http://valgrind.org)
* Support for compiling the tests with [AddressSanitizer](https://github.com/google/sanitizers) and [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html).
* `travis.yml` file for running unit tests on travis. (not enabled for the repository yet though [#102](https://github.com/DaveGamble/cJSON/issues/102)

Simplifications
---------------

After having unit tests for the parser function in place, I started refactoring the parser functions (as well as others) and making them easier to read and maintain.
* Use `strtod` from the standard library for parsing numbers ([0747669](https://github.com/DaveGamble/cJSON/commit/074766997246481dfc72bfa78f07898a2716473f))
* Use goto-fail in several parser functions ([#100](https://github.com/DaveGamble/cJSON/pull/100))
* Rewrite/restructure all of the parsing functions to be easier to understand and have less code paths doing the same as another. ([#109](https://github.com/DaveGamble/cJSON/pull/109))
* Simplify the buffer allocation strategy to always doubling the needed amount ([9f6fa94](https://github.com/DaveGamble/cJSON/commit/9f6fa94c91a87b71e4c6868dbf2ce431a48517b0))
* Combined `cJSON_AddItemToObject` and `cJSON_AddItemToObjectCS` to one function ([cf862d](https://github.com/DaveGamble/cJSON/commit/cf862d0fed7f9407e4b046d78d3d8050d2080d12))

Other changes
-------------
* Prevent the usage of incompatible C and header versions via preprocessor directive ([123bb1](https://github.com/DaveGamble/cJSON/commit/123bb1af7bfae41d805337fef4b41045ef6c7d25))
* Let CMake automatically detect compiler flags
* Add new compiler flags (`-Wundef`, `-Wswitch-default`, `-Wconversion`, `-fstack-protector-strong`) ([#98](https://github.com/DaveGamble/cJSON/pull/98))
* Change internal sizes from `int` to `size_t` ([ecd5678](https://github.com/DaveGamble/cJSON/commit/ecd5678527a6bc422da694e5be9e9979878fe6a0))
* Change internal strings from `char*` to `unsigned char*` ([28b9ba4](https://github.com/DaveGamble/cJSON/commit/28b9ba4334e0f7309e867e874a31f395c0ac2474))
* Add `const` in more places

1.2.1 (Jan 31, 2017)
=====
Fixes:
------
* Fixes a potential null pointer dereference in cJSON_Utils, discovered using clang's static analyzer by @bnason-nf, see [#96](https://github.com/DaveGamble/cJSON/issues/96)

1.2.0 (Jan 9, 2017)
=====
Features:
---------
* Add a new type of cJSON item for raw JSON and support printing it. Thanks @loigu, see [#65](https://github.com/DaveGamble/cJSON/pull/65), [#90](https://github.com/DaveGamble/cJSON/pull/90)

Fixes:
------
* Compiler warning if const is casted away, Thanks @gatzka, see [#83](https://github.com/DaveGamble/cJSON/pull/83)
* Fix compile error with strict-overflow on PowerPC, see [#85](https://github.com/DaveGamble/cJSON/issues/85)
* Fix typo in the README, thanks @MicroJoe, see [#88](https://github.com/DaveGamble/cJSON/pull/88)
* Add compile flag for compatibility with C++ compilers

1.1.0 (Dec 6, 2016)
=====
* Add a function `cJSON_PrintPreallocated` to print to a preallocated buffer, thanks @ChisholmKyle, see [#72](https://github.com/DaveGamble/cJSON/pull/72)
* More compiler warnings when using Clang or GCC, thanks @gatzka, see [#75](https://github.com/DaveGamble/cJSON/pull/75), [#78](https://github.com/DaveGamble/cJSON/pull/78)
* fixed a memory leak in `cJSON_Duplicate`, thanks @alperakcan, see [#81](https://github.com/DaveGamble/cJSON/pull/81)
* fix the `ENABLE_CUSTOM_COMPILER_FLAGS` cmake option

1.0.2 (Nov 25, 2016)
=====
* Rename internal boolean type, see [#71](https://github.com/DaveGamble/cJSON/issues/71).

1.0.1 (Nov 20, 2016)
=====
Small bugfix release.
* Fixes a bug with the use of the cJSON structs type in cJSON_Utils, see [d47339e](https://github.com/DaveGamble/cJSON/commit/d47339e2740360e6e0994527d5e4752007480f3a)
* improve code readability
* initialize all variables

1.0.0 (Nov 17, 2016)
=====
This is the first official versioned release of cJSON. It provides an API version for the shared library and improved Makefile and CMake build files.
