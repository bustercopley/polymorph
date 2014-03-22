PROGRAMS=nodes polymorph #tinyscheme
CC=gcc
CXX=g++
CFLAGS=-pedantic -Wall -Wextra
CXXFLAGS=-std=c++0x

nodes_FILENAME=nodes.exe
nodes_CFLAGS=
nodes_LDFLAGS=
nodes_SOURCE_PREFIX=src/nodes/
nodes_OBJECTS=main.o show_system.o snub_variance.o triangle.o rotor.o

polymorph_FILENAME=polymorph.scr
polymorph_CPPFLAGS=$(CONFIG_CPPFLAGS)
polymorph_CFLAGS=$(CONFIG_CFLAGS)
polymorph_CXXFLAGS=$(CONFIG_CXXFLAGS)
polymorph_LDFLAGS=$(CONFIG_LDFLAGS)
polymorph_LDLIBS=$(CONFIG_LDLIBS)
polymorph_EXTRA_OBJECTS=.obj/resources-res.o #.obj/tinyscheme-scheme.o
polymorph_SOURCE_PREFIX=src/
polymorph_OBJECTS=\
bump.o cmdline.o glinit.o graphics.o kdtree.o main.o markov.o \
memory.o model.o partition.o random.o rodrigues.o systems.o
# tinyscheme_SOURCE_PREFIX=tinyscheme/
# tinyscheme_CPPFLAGS=-include src/tinyscheme-config.h
# tinyscheme_CFLAGS=-Ofast -Wall -Wextra -Wno-switch -Wno-unused-parameter
# tinyscheme_OBJECTS=scheme.o

# Platform-specific variables. Platform prefix is x86_ or x64_ (autodetected).
PLATFORM=$(if $(findstring x86_64,$(shell gcc -dumpmachine)),x64,x86)
PLATFORM_CFLAGS=$($(PLATFORM)_CFLAGS)
PLATFORM_ENTRY_POINT=$($(PLATFORM)_ENTRY_POINT)

x64_ENTRY_POINT=custom_startup

# The leading underscore indicates the _cdecl calling convention.
x86_ENTRY_POINT=_custom_startup
# Don't assume 16-byte stack alignment (bug 40838).
x86_CFLAGS=-mpreferred-stack-boundary=2

CONFIG=TINY
CONFIG_CPPFLAGS=$($(CONFIG)_CPPFLAGS)
CONFIG_CFLAGS=$($(CONFIG)_CFLAGS)
CONFIG_CXXFLAGS=$($(CONFIG)_CXXFLAGS)
CONFIG_LDFLAGS=$($(CONFIG)_LDFLAGS)
CONFIG_LDLIBS=$($(CONFIG)_LDLIBS)

BASE_CPPFLAGS=-DUNICODE #-Itinyscheme
BASE_CFLAGS=$(PLATFORM_CFLAGS) -msse3 -mfpmath=sse -fno-ident -flto -Os
BASE_CXXFLAGS=-fno-exceptions -fno-rtti
BASE_LDFLAGS=-mwindows -s
BASE_LDLIBS=-lopengl32

TINY_CPPFLAGS=$(BASE_CPPFLAGS) -DTINY
TINY_CFLAGS=$(BASE_CFLAGS)
TINY_CXXFLAGS=$(BASE_CXXFLAGS)
TINY_LDFLAGS=$(BASE_LDFLAGS) -nodefaultlibs -nostartfiles -Xlinker --entry=$(PLATFORM_ENTRY_POINT) -Xlinker --disable-runtime-pseudo-reloc
TINY_LDLIBS=$(BASE_LDLIBS) -lgdi32 -luser32 -lkernel32

EXTRA_CLEAN=data

.PHONY: all test

all: $(polymorph_FILENAME)

test: all
	$(polymorph_FILENAME) -x

debug: all
	gdb --quiet --batch -ex run -ex bt full -ex quit --args $(polymorph_FILENAME) -x

SHADER_RESOURCES=\
.obj/vertex-shader.glsl.mini \
.obj/shared-geometry-shader.glsl.mini \
.obj/geometry-shader.glsl.mini \
.obj/snub-geometry-shader.glsl.mini \
.obj/fragment-shader.glsl.mini

$(SHADER_RESOURCES): | .obj

.obj/%.glsl.mini: src/%.glsl minify.pl
	c:\\strawberry\\perl\\bin\\perl minify.pl $< $@

.obj/resources-res.o: .obj/data $(SHADER_RESOURCES) polymorph.scr.manifest src/resources.rc
	windres -I.obj src/resources.rc .obj/resources-res.o

.obj/data: $(nodes_FILENAME) | .obj
	.\\$(nodes_FILENAME) .obj\\data>NUL

include program.mak
