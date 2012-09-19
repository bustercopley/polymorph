PROGRAMS=nodes polymorph #tinyscheme
SHELL=cmd
CC=gcc
CXX=g++
CFLAGS=-Wstrict-aliasing -fstrict-aliasing
CXXFLAGS=-std=c++0x

nodes_FILENAME=nodes.exe
nodes_CFLAGS=-pedantic-errors -Wall -Wextra
nodes_LDFLAGS=
nodes_SOURCE_PREFIX=src/nodes/
nodes_OBJECTS=main.o show_system.o snub_variance.o triangle.o rotor.o

polymorph_FILENAME=polymorph.scr
polymorph_CPPFLAGS=#-Itinyscheme
polymorph_CFLAGS=-pedantic-errors -Wall -Wextra -mstackrealign -msse4.2 -mfpmath=sse -Ofast
polymorph_CXXFLAGS=-fno-exceptions -fno-rtti
polymorph_LDFLAGS=-mwindows -s
polymorph_LDLIBS=-lopengl32
polymorph_EXTRA_OBJECTS=resources-res.o #.obj/tinyscheme-scheme.o
polymorph_SOURCE_PREFIX=src/
polymorph_OBJECTS=\
bounce.o cmdline.o glprocs.o graphics.o kdtree.o main.o markov.o memory.o \
model.o object.o partition.o random.o rodrigues.o system_ref.o
tinyscheme_SOURCE_PREFIX=tinyscheme/
tinyscheme_CPPFLAGS=-DSTANDALONE=0 -DUSE_NO_FEATURES=1 -DUSE_STRLWR=0
tinyscheme_CFLAGS=-Os -Wall -Wextra -Wno-switch -Wno-unused-parameter
tinyscheme_OBJECTS=scheme.o

EXTRA_CLEAN=data resources-res.o

.PHONY: all test
all: $(polymorph_FILENAME)

test: all
	$(polymorph_FILENAME) -x

resources-res.o: data polymorph.scr.manifest resources.rc
	windres resources.rc resources-res.o

data: nodes.exe
	.\\$(nodes_FILENAME) data

include program.mak
