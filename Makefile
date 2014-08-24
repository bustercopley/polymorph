PROGRAMS=polymorph
PLATFORM=x64
CONFIG=tiny

OLDPATH:=$(PATH)
PATH=$(PLATFORM_PATH);$(OLDPATH)

CC=gcc
CXX=g++
CFLAGS=-pedantic -Wall -Wextra
CXXFLAGS=-std=c++1y

polymorph_FILENAME=$($(PLATFORM)_$(CONFIG)_APPNAME).scr
polymorph_OBJDIR=.obj/polymorph/$(PLATFORM)/$(CONFIG)
polymorph_CPPFLAGS=$(CONFIG_CPPFLAGS)
polymorph_CFLAGS=$(CONFIG_CFLAGS)
polymorph_CXXFLAGS=$(CONFIG_CXXFLAGS)
polymorph_LDFLAGS=$(CONFIG_LDFLAGS)
polymorph_LDLIBS=$(CONFIG_LDLIBS)
polymorph_RESFLAGS=-DPLATFORM_CONFIG=$(PLATFORM)_$(CONFIG) -I.obj/polymorph -I.obj/polymorph/$(PLATFORM)
polymorph_EXTRA_OBJECTS=.obj/polymorph/$(PLATFORM)/$(CONFIG)/resources-res.o
polymorph_SOURCE_PREFIX=src/
polymorph_OBJECTS=\
arguments.o bump.o dialog.o glinit.o graphics.o kdtree.o main.o markov.o memory.o \
model.o partition.o polymorph.o random.o reposition.o rodrigues.o settings.o systems.o make_system.o

common_CPPFLAGS=-DUNICODE -D_UNICODE
common_CFLAGS=-msse3 -mfpmath=sse -fno-ident
common_CXXFLAGS=-fno-rtti -fno-exceptions -ffast-math
common_LDFLAGS=-mwindows -municode
common_LDLIBS=-lopengl32 -lcomctl32 -lshell32

base_CPPFLAGS=$(common_CPPFLAGS)
base_CFLAGS=$(common_CFLAGS) -flto -Os
base_CXXFLAGS=$(common_CXXFLAGS)
base_LDFLAGS=$(common_LDFLAGS) -s
base_LDLIBS=$(common_LDLIBS)

tiny_CPPFLAGS=$(base_CPPFLAGS) -DTINY
tiny_CFLAGS=$(base_CFLAGS) -fno-asynchronous-unwind-tables
tiny_CXXFLAGS=$(base_CXXFLAGS)
tiny_LDFLAGS=$(base_LDFLAGS) -nostdlib -Wl,--disable-runtime-pseudo-reloc --entry=$(PLATFORM_ENTRY_POINT)
tiny_LDLIBS=$(base_LDLIBS) -lgdi32 -ladvapi32 -luser32 -lkernel32

debug_CPPFLAGS=$(common_CPPFLAGS)
debug_CFLAGS=$(common_CFLAGS) -g -ggdb
debug_CXXFLAGS=$(common_CXXFLAGS)
debug_LDFLAGS=$(common_LDFLAGS)
debug_LDLIBS=$(common_LDLIBS)

x86_PATH=C:\mingw32\bin
x86_ENTRY_POINT=_custom_startup

x64_PATH=C:\mingw64\bin
x64_ENTRY_POINT=custom_startup

x64_base_APPNAME=Polymorph-base
x86_base_APPNAME=Polymorph-x86-base
x64_tiny_APPNAME=Polymorph
x86_tiny_APPNAME=Polymorph-x86
x64_debug_APPNAME=Polymorph-debug
x86_debug_APPNAME=Polymorph-x86-debug

PLATFORM_ENTRY_POINT=$($(PLATFORM)_ENTRY_POINT)
PLATFORM_PATH=$($(PLATFORM)_PATH)

CONFIG_CPPFLAGS=$($(CONFIG)_CPPFLAGS)
CONFIG_CFLAGS=$($(CONFIG)_CFLAGS)
CONFIG_CXXFLAGS=$($(CONFIG)_CXXFLAGS)
CONFIG_LDFLAGS=$($(CONFIG)_LDFLAGS)
CONFIG_LDLIBS=$($(CONFIG)_LDLIBS)

EXTRA_CLEAN=Polymorph*.scr

.PHONY: all test debug

all: $(polymorph_FILENAME)

test: all
	$(polymorph_FILENAME)

debug: all
	$(PLATFORM_PATH)\gdb --quiet --batch -ex run -ex bt full -ex quit --args $(polymorph_FILENAME) -x

SHADER_RESOURCES=\
.obj/polymorph/vertex-shader.glsl.mini \
.obj/polymorph/shared-geometry-shader.glsl.mini \
.obj/polymorph/geometry-shader.glsl.mini \
.obj/polymorph/snub-geometry-shader.glsl.mini \
.obj/polymorph/fragment-shader.glsl.mini

RESOURCES=$(SHADER_RESOURCES) src/polymorph.scr.manifest

.obj/polymorph:
	-md .obj\polymorph

.obj/polymorph/$(PLATFORM):
	-md .obj\polymorph\$(PLATFORM)

.obj/polymorph/%.glsl.mini: src/%.glsl minify.pl | .obj/polymorph
	c:\strawberry\perl\bin\perl minify.pl $< $@

.obj/polymorph/$(PLATFORM)/$(CONFIG)/resources-res.o: $(RESOURCES) src/resources.h

include program.mak
include build-all.mak
