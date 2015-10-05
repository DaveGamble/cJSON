OBJ = cJSON.o
LIBNAME = libcjson
TESTS = test

PREFIX ?= /usr/local
INCLUDE_PATH ?= include/cjson
LIBRARY_PATH ?= lib

INSTALL_INCLUDE_PATH = $(DESTDIR)$(PREFIX)/$(INCLUDE_PATH)
INSTALL_LIBRARY_PATH = $(DESTDIR)$(PREFIX)/$(LIBRARY_PATH)

INSTALL ?= cp -a

R_CFLAGS = -fpic $(CFLAGS) -Wall -Werror -Wstrict-prototypes -Wwrite-strings -D_POSIX_C_SOURCE=200112L

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo false')

## shared lib
DYLIBNAME = $(LIBNAME).so 
DYLIBCMD = $(CC) -shared -o $(DYLIBNAME)

## create dynamic (shared) library on Darwin (base OS for MacOSX and IOS)
ifeq (Darwin, $(uname_S))
  DYLIBNAME = $(LIBNAME).dylib
endif
## create dyanmic (shared) library on SunOS
ifeq (SunOS, $(uname_S))
  DYLIBCMD = $(CC) -G -o $(DYLIBNAME)
  INSTALL = cp -r
endif

## static lib
STLIBNAME = $(LIBNAME).a

.PHONY: all clean install

all: $(DYLIBNAME) $(STLIBNAME) $(TESTS)

$(DYLIBNAME): $(OBJ)
		$(DYLIBCMD) $< $(LDFLAGS)
	
$(STLIBNAME): $(OBJ)
		ar rcs $@ $<

$(OBJ): cJSON.c cJSON.h 

.c.o:
		$(CC) -ansi -pedantic -c $(R_CFLAGS) $<

$(TESTS): cJSON.c cJSON.h test.c
		$(CC)  cJSON.c test.c -o test -lm -I.

install: $(DYLIBNAME) $(STLIBNAME)
		mkdir -p $(INSTALL_LIBRARY_PATH) $(INSTALL_INCLUDE_PATH)
		$(INSTALL) cJSON.h $(INSTALL_INCLUDE_PATH)
		$(INSTALL) $(DYLIBNAME) $(INSTALL_LIBRARY_PATH)
		$(INSTALL) $(STLIBNAME) $(INSTALL_LIBRARY_PATH)

clean: 
		rm -rf $(DYLIBNAME) $(STLIBNAME) $(TESTS) *.o
