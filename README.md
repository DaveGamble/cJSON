# cJSON

Ultralightweight JSON parser in ANSI C.

## Table of contents
* [License](#license)
* [Usage](#usage)
  * [Welcome to cJSON](#welcome-to-cjson)
  * [Building](#building)
  * [Including cJSON](#including-cjson)
  * [Data Structure](#data-structure)
  * [Parsing JSON](#parsing-json)
  * [Printing JSON](#printing-json)
  * [Some JSON](#some-json)
  * [Here's the structure](#heres-the-structure)
  * [Caveats](#caveats)
  * [Enjoy cJSON!](#enjoy-cjson)

## License

MIT License

>  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors
>
>  Permission is hereby granted, free of charge, to any person obtaining a copy
>  of this software and associated documentation files (the "Software"), to deal
>  in the Software without restriction, including without limitation the rights
>  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
>  copies of the Software, and to permit persons to whom the Software is
>  furnished to do so, subject to the following conditions:
>
>  The above copyright notice and this permission notice shall be included in
>  all copies or substantial portions of the Software.
>
>  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
>  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
>  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
>  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
>  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
>  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
>  THE SOFTWARE.

## Usage

### Welcome to cJSON.

cJSON aims to be the dumbest possible parser that you can get your job done with.
It's a single file of C, and a single header file.

JSON is described best here: http://www.json.org/
It's like XML, but fat-free. You use it to move data around, store things, or just
generally represent your program's state.

As a library, cJSON exists to take away as much legwork as it can, but not get in your way.
As a point of pragmatism (i.e. ignoring the truth), I'm going to say that you can use it
in one of two modes: Auto and Manual. Let's have a quick run-through.

I lifted some JSON from this page: http://www.json.org/fatfree.html
That page inspired me to write cJSON, which is a parser that tries to share the same
philosophy as JSON itself. Simple, dumb, out of the way.

### Building

There are several ways to incorporate cJSON into your project.

#### copying the source
Because the entire library is only one C file and one header file, you can just copy `cJSON.h` and `cJSON.c` to your projects source and start using it.

cJSON is written in ANSI C (C89) in order to support as many platforms and compilers as possible.

#### CMake
With CMake, cJSON supports a full blown build system. This way you get the most features. CMake with an equal or higher version than 2.8.5 is supported. With CMake it is recommended to do an out of tree build, meaning the compiled files are put in a directory separate from the source files. So in order to build cJSON with CMake on a Unix platform, make a `build` directory and run CMake inside it.

```
mkdir build
cd build
cmake ..
```

This will create a Makefile and a bunch of other files. You can then compile it:

```
make
```

And install it with `make install` if you want. By default it installs the headers `/usr/local/include/cjson` and the libraries to `/usr/local/lib`. It also installs files for pkg-config to make it easier to detect and use an existing installation of CMake. And it installs CMake config files, that can be used by other CMake based projects to discover the library.

You can change the build process with a list of different options that you can pass to CMake. Turn them on with `On` and off with `Off`:
* `-DENABLE_CJSON_TEST=On`: Enable building the tests. (on by default)
* `-DENABLE_CJSON_UTILS=On`: Enable building cJSON_Utils. (off by default)
* `-DENABLE_TARGET_EXPORT=On`: Enable the export of CMake targets. Turn off if it makes problems. (on by default)
* `-DENABLE_CUSTOM_COMPILER_FLAGS=On`: Enable custom compiler flags (currently for Clang, GCC and MSVC). Turn off if it makes problems. (on by default)
* `-DENABLE_VALGRIND=On`: Run tests with [valgrind](http://valgrind.org). (off by default)
* `-DENABLE_SANITIZERS=On`: Compile cJSON with [AddressSanitizer](https://github.com/google/sanitizers/wiki/AddressSanitizer) and [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html) enabled (if possible). (off by default)
* `-DENABLE_SAFE_STACK`: Enable the [SafeStack](https://clang.llvm.org/docs/SafeStack.html) instrumentation pass. Currently only works with the Clang compiler. (off by default)
* `-DBUILD_SHARED_LIBS=On`: Build the shared libraries. (on by default)
* `-DBUILD_SHARED_AND_STATIC_LIBS=On`: Build both shared and static libraries. (off by default)
* `-DCMAKE_INSTALL_PREFIX=/usr`: Set a prefix for the installation.
* `-DENABLE_LOCALES=On`: Enable the usage of localeconv method. ( on by default )
* `-DCJSON_OVERRIDE_BUILD_SHARED_LIBS=On`: Enable overriding the value of `BUILD_SHARED_LIBS` with `-DCJSON_BUILD_SHARED_LIBS`.

If you are packaging cJSON for a distribution of Linux, you would probably take these steps for example:
```
mkdir build
cd build
cmake .. -DENABLE_CJSON_UTILS=On -DENABLE_CJSON_TEST=Off -DCMAKE_INSTALL_PREFIX=/usr
make
make DESTDIR=$pkgdir install
```

#### Makefile
If you don't have CMake available, but still have GNU make. You can use the makefile to build cJSON:

Run this command in the directory with the source code and it will automatically compile static and shared libraries and a little test program.

```
make all
```

If you want, you can install the compiled library to your system using `make install`. By default it will install the headers in `/usr/local/include/cjson` and the libraries in `/usr/local/lib`. But you can change this behavior by setting the `PREFIX` and `DESTDIR` variables: `make PREFIX=/usr DESTDIR=temp install`.

### Including cJSON
If you installed it via CMake or the Makefile, you can include cJSON like this:

```c
#include <cjson/cJSON.h>
```

### Data Structure

cJSON represents JSON data using the `cJSON` struct data type:

```c
/* The cJSON structure: */
typedef struct cJSON
{
    struct cJSON *next;
    struct cJSON *prev;
    struct cJSON *child;
    int type;
    char *valuestring;
    /* writing to valueint is DEPRECATED, use cJSON_SetNumberValue instead */
    int valueint;
    double valuedouble;
    char *string;
} cJSON;
```

An item of this type represents a JSON value. The type is stored in `type` as a bit-flag (**this means that you cannot find out the type by just comparing the value of `type`**).

To check the type of an item, use the corresponding `cJSON_Is...` function. It does a `NULL` check followed by a type check and returns a boolean value if the item is of this type.

The type can be one of the following:
* `cJSON_Invalid` (check with `cJSON_IsInvalid`): Represents an invalid item that doesn't contain any value. You automatically have this type if you set the item to all zero bytes.
* `cJSON_False` (check with `cJSON_IsFalse`): Represents a `false` boolean value. You can also check for boolean values in general with `cJSON_IsBool`.
* `cJSON_True` (check with `cJSON_IsTrue`): Represents a `true` boolean value. You can also check for boolean values in general with `cJSON_IsBool`.
* `cJSON_NULL` (check with `cJSON_IsNull`): Represents a `null` value.
* `cJSON_Number` (check with `cJSON_IsNumber`): Represents a number value. The value is stored as a double in `valuedouble` and also in `valueint`. If the number is outside of the range of an integer, `INT_MAX` or `INT_MIN` are used for `valueint`.
* `cJSON_String` (check with `cJSON_IsString`): Represents a string value. It is stored in the form of a zero terminated string in `valuestring`.
* `cJSON_Array` (check with `cJSON_IsArray`): Represent an array value. This is implemented by pointing `child` to a linked list of `cJSON` items that represent the values in the array. The elements are linked together using `next` and `prev`, where the first element has `prev == NULL` and the last element `next == NULL`.
* `cJSON_Object` (check with `cJSON_IsObject`): Represents an object value. Objects are stored same way as an array, the only difference is that the items in the object store their keys in `string`.
* `cJSON_Raw` (check with `cJSON_IsRaw`): Represents any kind of JSON that is stored as a zero terminated array of characters in `valuestring`. This can be used, for example, to avoid printing the same static JSON over and over again to save performance. cJSON will never create this type when parsing. Also note that cJSON doesn't check if it is valid JSON.

Additionally there are the following two flags:
* `cJSON_IsReference`: Specifies that the item that `child` points to and/or `valuestring` is not owned by this item, it is only a reference. So `cJSON_Delete` and other functions will only deallocate this item, not it's children/valuestring.
* `cJSON_StringIsConst`: This means that `string` points to a constant string. This means that `cJSON_Delete` and other functions will not try to deallocate `string`.

### Parsing JSON

Given some JSON in a zero terminated string, you can parse it with `cJSON_Parse`.

```c
cJSON *json = cJSON_Parse(string);
```

It will parse the JSON and allocate a tree of `cJSON` items that represents it. Once it returns, you are fully responsible for deallocating it after use with `cJSON_Delete`.

The allocator used by `cJSON_Parse` is `malloc` and `free` by default but can be changed (globally) with `cJSON_InitHooks`.

If an error occurs a pointer to the position of the error in the input string can be accessed using `cJSON_GetErrorPtr`. Note though that this can produce race conditions in multithreading scenarios, in that case it is better to use `cJSON_ParseWithOpts` with `return_parse_end`.
By default, characters in the input string that follow the parsed JSON will not be considered as an error.

If you want more options, use `cJSON_ParseWithOpts(const char *value, const char **return_parse_end, cJSON_bool require_null_terminated)`.
`return_parse_end` returns a pointer to the end of the JSON in the input string or the position that an error occurs at (thereby replacing `cJSON_GetErrorPtr` in a thread safe way). `require_null_terminated`, if set to `1` will make it an error if the input string contains data after the JSON.

### Printing JSON

Given a tree of `cJSON` items, you can print them as a string using `cJSON_Print`.

```c
char *string = cJSON_Print(json);
```

It will allocate a string and print a JSON representation of the tree into it. Once it returns, you are fully responsible for deallocating it after use with your allocator. (usually `free`, depends on what has been set with `cJSON_InitHooks`).

`cJSON_Print` will print with whitespace for formatting. If you want to print without formatting, use `cJSON_PrintUnformatted`.

If you have a rough idea of how big your resulting string will be, you can use `cJSON_PrintBuffered(const cJSON *item, int prebuffer, cJSON_bool fmt)`. `fmt` is a boolean to turn formatting with whitespace on and off. `prebuffer` specifies the first buffer size to use for printing. `cJSON_Print` currently uses 256 bytes for it's first buffer size. Once printing runs out of space, a new buffer is allocated and the old gets copied over before printing is continued.

These dynamic buffer allocations can be completely avoided by using `cJSON_PrintPreallocated(cJSON *item, char *buffer, const int length, const cJSON_bool format)`. It takes a buffer to a pointer to print to and it's length. If the length is reached, printing will fail and it returns `0`. In case of success, `1` is returned. Note that you should provide 5 bytes more than is actually needed, because cJSON is not 100% accurate in estimating if the provided memory is enough.

### Some JSON:

```json
{
    "name": "Jack (\"Bee\") Nimble",
    "format": {
        "type":       "rect",
        "width":      1920,
        "height":     1080,
        "interlace":  false,
        "frame rate": 24
    }
}
```

Assume that you got this from a file, a webserver, or magic JSON elves, whatever,
you have a `char *` to it. Everything is a `cJSON` struct.
Get it parsed:

```c
cJSON * root = cJSON_Parse(my_json_string);
```

This is an object. We're in C. We don't have objects. But we do have structs.
What's the framerate?

```c
cJSON *format = cJSON_GetObjectItemCaseSensitive(root, "format");
cJSON *framerate_item = cJSON_GetObjectItemCaseSensitive(format, "frame rate");
double framerate = 0;
if (cJSON_IsNumber(framerate_item))
{
  framerate = framerate_item->valuedouble;
}
```

Want to change the framerate?

```c
cJSON *framerate_item = cJSON_GetObjectItemCaseSensitive(format, "frame rate");
cJSON_SetNumberValue(framerate_item, 25);
```

Back to disk?

```c
char *rendered = cJSON_Print(root);
```

Finished? Delete the root (this takes care of everything else).

```c
cJSON_Delete(root);
```

That's AUTO mode. If you're going to use Auto mode, you really ought to check pointers
before you dereference them. If you want to see how you'd build this struct in code?

```c
cJSON *root;
cJSON *fmt;
root = cJSON_CreateObject();
cJSON_AddItemToObject(root, "name", cJSON_CreateString("Jack (\"Bee\") Nimble"));
cJSON_AddItemToObject(root, "format", fmt = cJSON_CreateObject());
cJSON_AddStringToObject(fmt, "type", "rect");
cJSON_AddNumberToObject(fmt, "width", 1920);
cJSON_AddNumberToObject(fmt, "height", 1080);
cJSON_AddFalseToObject (fmt, "interlace");
cJSON_AddNumberToObject(fmt, "frame rate", 24);
```

Hopefully we can agree that's not a lot of code? There's no overhead, no unnecessary setup.
Look at `test.c` for a bunch of nice examples, mostly all ripped off the [json.org](http://json.org) site, and
a few from elsewhere.

What about manual mode? First up you need some detail.
Let's cover how the `cJSON` objects represent the JSON data.
cJSON doesn't distinguish arrays from objects in handling; just type.
Each `cJSON` has, potentially, a child, siblings, value, a name.

* The `root` object has: *Object* Type and a Child
* The Child has name "name", with value "Jack ("Bee") Nimble", and a sibling:
* Sibling has type *Object*, name "format", and a child.
* That child has type *String*, name "type", value "rect", and a sibling:
* Sibling has type *Number*, name "width", value 1920, and a sibling:
* Sibling has type *Number*, name "height", value 1080, and a sibling:
* Sibling has type *False*, name "interlace", and a sibling:
* Sibling has type *Number*, name "frame rate", value 24

### Here's the structure:

```c
typedef struct cJSON {
    struct cJSON *next,*prev;
    struct cJSON *child;

    int type;

    char *valuestring;
    int valueint; /* writing to valueint is DEPRECATED, please use cJSON_SetNumberValue instead */
    double valuedouble;

    char *string;
} cJSON;
```

By default all values are 0 unless set by virtue of being meaningful.

`next`/`prev` is a doubly linked list of siblings. `next` takes you to your sibling,
`prev` takes you back from your sibling to you.
Only objects and arrays have a `child`, and it's the head of the doubly linked list.
A `child` entry will have `prev == 0`, but next potentially points on. The last sibling has `next == 0`.
The type expresses *Null*/*True*/*False*/*Number*/*String*/*Array*/*Object*, all of which are `#defined` in
`cJSON.h`.

A *Number* has `valueint` and `valuedouble`. `valueint` is a relict of the past, so always use `valuedouble`.

Any entry which is in the linked list which is the child of an object will have a `string`
which is the "name" of the entry. When I said "name" in the above example, that's `string`.
`string` is the JSON name for the 'variable name' if you will.

Now you can trivially walk the lists, recursively, and parse as you please.
You can invoke `cJSON_Parse` to get cJSON to parse for you, and then you can take
the root object, and traverse the structure (which is, formally, an N-tree),
and tokenise as you please. If you wanted to build a callback style parser, this is how
you'd do it (just an example, since these things are very specific):

```c
void parse_and_callback(cJSON *item, const char *prefix)
{
    while (item)
    {
        char *newprefix = malloc(strlen(prefix) + strlen(item->string) + 2);
        sprintf(newprefix, "%s/%s", prefix, item->string);
        int dorecurse = callback(newprefix, item->type, item);
        if (item->child && dorecurse)
        {
            parse_and_callback(item->child, newprefix);
        }
        item = item->next;
        free(newprefix);
    }
}
```

The `prefix` process will build you a separated list, to simplify your callback handling.
The `dorecurse` flag would let the callback decide to handle sub-arrays on it's own, or
let you invoke it per-item. For the item above, your callback might look like this:

```c
int callback(const char *name, int type, cJSON *item)
{
    if (!strcmp(name, "name"))
    {
        /* populate name */
    }
    else if (!strcmp(name, "format/type"))
    {
        /* handle "rect" */ }
    else if (!strcmp(name, "format/width"))
    {
        /* 800 */
    }
    else if (!strcmp(name, "format/height"))
    {
        /* 600 */
    }
    else if (!strcmp(name, "format/interlace"))
    {
        /* false */
    }
    else if (!strcmp(name, "format/frame rate"))
    {
        /* 24 */
    }

    return 1;
}
```

Alternatively, you might like to parse iteratively.
You'd use:

```c
void parse_object(cJSON *item)
{
    int i;
    for (i = 0; i < cJSON_GetArraySize(item); i++)
    {
        cJSON *subitem = cJSON_GetArrayItem(item, i);
        // handle subitem
    }
}
```

Or, for PROPER manual mode:

```c
void parse_object(cJSON *item)
{
    cJSON *subitem = item->child;
    while (subitem)
    {
        // handle subitem
        if (subitem->child)
        {
            parse_object(subitem->child);
        }

        subitem = subitem->next;
    }
}
```

Of course, this should look familiar, since this is just a stripped-down version
of the callback-parser.

This should cover most uses you'll find for parsing. The rest should be possible
to infer.. and if in doubt, read the source! There's not a lot of it! ;)

In terms of constructing JSON data, the example code above is the right way to do it.
You can, of course, hand your sub-objects to other functions to populate.
Also, if you find a use for it, you can manually build the objects.
For instance, suppose you wanted to build an array of objects?

```c
cJSON *objects[24];

cJSON *Create_array_of_anything(cJSON **items, int num)
{
    int i;
    cJSON *prev;
    cJSON *root = cJSON_CreateArray();
    for (i = 0; i < 24; i++)
    {
        if (!i)
        {
            root->child = objects[i];
        }
        else
        {
            prev->next = objects[i];
            objects[i]->prev = prev;
        }

        prev = objects[i];
    }

    return root;
}
```

and simply: `Create_array_of_anything(objects, 24);`

cJSON doesn't make any assumptions about what order you create things in.
You can attach the objects, as above, and later add children to each
of those objects.

As soon as you call `cJSON_Print`, it renders the structure to text.

The `test.c` code shows how to handle a bunch of typical cases. If you uncomment
the code, it'll load, parse and print a bunch of test files, also from [json.org](http://json.org),
which are more complex than I'd care to try and stash into a `const char array[]`.

### Caveats

#### Zero Character

cJSON doesn't support strings that contain the zero character `'\0'` or `\u0000`. This is impossible with the current API because strings are zero terminated.

#### Character Encoding

cJSON only supports UTF-8 encoded input. In most cases it doesn't reject invalid UTF-8 as input though, it just propagates it through as is. As long as the input doesn't contain invalid UTF-8, the output will always be valid UTF-8.

#### C Standard

cJSON is written in ANSI C (or C89, C90). If your compiler or C library doesn't follow this standard, correct behavior is not guaranteed.

NOTE: ANSI C is not C++ therefore it shouldn't be compiled with a C++ compiler. You can compile it with a C compiler and link it with your C++ code however. Although compiling with a C++ compiler might work, correct behavior is not guaranteed.

#### Floating Point Numbers

cJSON does not officially support any `double` implementations other than IEEE754 double precision floating point numbers. It might still work with other implementations but bugs with these will be considered invalid.

The maximum length of a floating point literal that cJSON supports is currently 63 characters.

#### Deep Nesting Of Arrays And Objects

cJSON doesn't support arrays and objects that are nested too deeply because this would result in a stack overflow. To prevent this cJSON limits the depth to `CJSON_NESTING_LIMIT` which is 1000 by default but can be changed at compile time.

#### Thread Safety

In general cJSON is **not thread safe**.

However it is thread safe under the following conditions:
* `cJSON_GetErrorPtr` is never used (the `return_parse_end` parameter of `cJSON_ParseWithOpts` can be used instead)
* `cJSON_InitHooks` is only ever called before using cJSON in any threads.
* `setlocale` is never called before all calls to cJSON functions have returned.

#### Case Sensitivity

When cJSON was originally created, it didn't follow the JSON standard and didn't make a distinction between uppercase and lowercase letters. If you want the correct, standard compliant, behavior, you need to use the `CaseSensitive` functions where available.

# Enjoy cJSON!

- Dave Gamble, Aug 2009
- [cJSON contributors](CONTRIBUTORS.md)
