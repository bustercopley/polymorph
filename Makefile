PROGRAMS=nodes polymorph #tinyscheme
CC=gcc
CXX=g++
CFLAGS=$(MACHINE_CFLAGS) -fstrict-aliasing -pedantic -Wall -Wextra -Wstrict-aliasing
CXXFLAGS=-std=c++0x

X86_64=$(findstring x86_64,$(shell gcc -dumpmachine))

# On 32-bit Windows, prevent gcc from assuming the stack is 16-byte aligned (bug 40838).
MACHINE_CFLAGS=$(if $(X86_64),,-mpreferred-stack-boundary=2)

nodes_FILENAME=nodes.exe
nodes_CFLAGS=
nodes_LDFLAGS=
nodes_SOURCE_PREFIX=src/nodes/
nodes_OBJECTS=main.o show_system.o snub_variance.o triangle.o rotor.o

polymorph_FILENAME=polymorph.scr
polymorph_CPPFLAGS=-DUNICODE #-Itinyscheme
polymorph_CFLAGS=-msse4.2 -mfpmath=sse -flto -Os
polymorph_CXXFLAGS=-fno-exceptions -fno-rtti
polymorph_LDFLAGS=-mwindows -s
polymorph_LDLIBS=-lopengl32
polymorph_EXTRA_OBJECTS=.obj/resources-res.o #.obj/tinyscheme-scheme.o
polymorph_SOURCE_PREFIX=src/
polymorph_OBJECTS=\
bump.o cmdline.o glinit.o graphics.o kdtree.o main.o markov.o \
memory.o model.o partition.o random.o rodrigues.o systems.o
# tinyscheme_SOURCE_PREFIX=tinyscheme/
# tinyscheme_CPPFLAGS=-include src/tinyscheme-config.h
# tinyscheme_CFLAGS=-Ofast -Wall -Wextra -Wno-switch -Wno-unused-parameter
# tinyscheme_OBJECTS=scheme.o

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

.obj/resources-res.o: .obj/data $(SHADER_RESOURCES) polymorph.scr.manifest resources.rc
	windres resources.rc .obj/resources-res.o

.obj/data: $(nodes_FILENAME) | .obj
	.\\$(nodes_FILENAME) .obj\\data>NUL

include program.mak
