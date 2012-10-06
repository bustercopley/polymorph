PROGRAMS=nodes polymorph tinyscheme
SHELL=cmd
CC=gcc
CXX=g++
CFLAGS=-fstrict-aliasing -pedantic -Wall -Wextra -Wstrict-aliasing
CXXFLAGS=-std=c++0x

nodes_FILENAME=nodes.exe
nodes_CFLAGS=
nodes_LDFLAGS=
nodes_SOURCE_PREFIX=src/nodes/
nodes_OBJECTS=main.o show_system.o snub_variance.o triangle.o rotor.o

polymorph_FILENAME=polymorph.scr
polymorph_CPPFLAGS=-Itinyscheme
polymorph_CFLAGS=-mstackrealign -msse4.2 -mfpmath=sse -Ofast
polymorph_CXXFLAGS=-fno-exceptions -fno-rtti
polymorph_LDFLAGS=-mwindows -s
polymorph_LDLIBS=-lopengl32
polymorph_EXTRA_OBJECTS=.obj/resources-res.o .obj/tinyscheme-scheme.o
polymorph_SOURCE_PREFIX=src/
polymorph_OBJECTS=\
cmdline.o glprocs.o graphics.o kdtree.o main.o markov.o \
memory.o model.o partition.o random.o rodrigues.o systems.o
tinyscheme_SOURCE_PREFIX=tinyscheme/
tinyscheme_CPPFLAGS=-include src/tinyscheme.h
tinyscheme_CFLAGS=-Ofast -Wall -Wextra -Wno-switch -Wno-unused-parameter
tinyscheme_OBJECTS=scheme.o

EXTRA_CLEAN=data .obj/resources-res.o

.PHONY: all test shaders

all: $(polymorph_FILENAME)

test: all
	$(polymorph_FILENAME) -x

shaders: \
src/vertex-shader.glsl \
src/shared-geometry-shader.glsl \
src/geometry-shader.glsl \
src/snub-geometry-shader.glsl \
src/antisnub-geometry-shader.glsl \
src/fragment-shader.glsl

.obj/resources-res.o: data shaders polymorph.scr.manifest resources.rc | .obj
	windres resources.rc .obj/resources-res.o

data: nodes.exe
	.\\$(nodes_FILENAME) data>NUL

include program.mak
