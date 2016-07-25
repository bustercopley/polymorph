SHELL=cmd
PROGRAMS=polymorph
PLATFORM=x64
CONFIG=tiny

PLATFORMS=x64 x86
CONFIGS=tiny base debug
$(if $(filter $(CONFIG),$(CONFIGS)),,$(error Bad config "$(CONFIG)"))
$(if $(filter $(PLATFORM),$(PLATFORMS)),,$(error Bad platform "$(PLATFORM)"))

PATH=$(PLATFORM_PATH)

CC=gcc
CXX=g++
CFLAGS=-pedantic -Wall -Wextra
CXXFLAGS=-std=c++1z

polymorph_FILENAME=$($(PLATFORM)_$(CONFIG)_APPNAME).scr
polymorph_OBJDIR=.obj/$(PLATFORM)/$(CONFIG)
polymorph_CPPFLAGS=$($(CONFIG)_CPPFLAGS)
polymorph_CFLAGS=$($(CONFIG)_CFLAGS)
polymorph_CXXFLAGS=$($(CONFIG)_CXXFLAGS)
polymorph_LDFLAGS=$($(CONFIG)_LDFLAGS)
polymorph_LDLIBS=$($(CONFIG)_LDLIBS)
polymorph_RESFLAGS=-DPLATFORM_CONFIG=$(PLATFORM)_$(CONFIG) -I$(SHADER_DIRECTORY)
polymorph_SOURCE_PREFIX=src/
polymorph_OBJECTS=\
arguments.o bump.o dialog.o glinit.o graphics.o kdtree.o main.o markov.o memory.o \
model.o partition.o polymorph.o random.o reposition.o resources.o rodrigues.o \
settings.o systems.o make_system.o

common_CPPFLAGS=-DUNICODE
common_CFLAGS=-g -msse3 -mfpmath=sse -mno-stackrealign -fno-ident -fno-fast-math
common_CXXFLAGS=-fno-rtti -fno-exceptions
common_LDFLAGS=-mwindows -municode
common_LDLIBS=-lopengl32 -lcomctl32 -lshell32

base_CPPFLAGS=$(common_CPPFLAGS) -DENABLE_PRINT -DENABLE_GLCHECK -DENABLE_GLDEBUG
base_CFLAGS=$(common_CFLAGS) -flto -Os
base_CXXFLAGS=$(common_CXXFLAGS)
base_LDFLAGS=$(common_LDFLAGS)
base_LDLIBS=$(common_LDLIBS)
base_SHADERS=minified

tiny_CPPFLAGS=$(common_CPPFLAGS) -DTINY
tiny_CFLAGS=$(common_CFLAGS) -flto -Os -fno-asynchronous-unwind-tables
tiny_CXXFLAGS=$(common_CXXFLAGS)
tiny_LDFLAGS=$(common_LDFLAGS) -nostdlib -Wl,--disable-runtime-pseudo-reloc --entry=$(PLATFORM_ENTRY_POINT)
tiny_LDLIBS=$(common_LDLIBS) -lgdi32 -ladvapi32 -luser32 -lkernel32
tiny_SHADERS=minified

debug_CPPFLAGS=$(common_CPPFLAGS) -DENABLE_PRINT -DENABLE_GLCHECK -DENABLE_GLDEBUG
debug_CFLAGS=$(common_CFLAGS) -Og
debug_CXXFLAGS=$(common_CXXFLAGS)
debug_LDFLAGS=$(common_LDFLAGS)
debug_LDLIBS=$(common_LDLIBS)
debug_SHADERS=full

x86_PATH=C:\msys64\mingw32\bin
x86_ENTRY_POINT=_custom_startup

x64_PATH=C:\msys64\mingw64\bin
x64_ENTRY_POINT=custom_startup

x64_base_APPNAME=Polymorph-base
x86_base_APPNAME=Polymorph-x86-base
x64_tiny_APPNAME=Polymorph
x86_tiny_APPNAME=Polymorph-x86
x64_debug_APPNAME=Polymorph-debug
x86_debug_APPNAME=Polymorph-x86-debug

PLATFORM_ENTRY_POINT=$($(PLATFORM)_ENTRY_POINT)
PLATFORM_PATH=$($(PLATFORM)_PATH)

EXTRA_CLEAN=Polymorph*.scr

.PHONY: all test debug

all: $(polymorph_FILENAME) dump

test: all
	$(polymorph_FILENAME) $(ARG)

testx: all
	$(polymorph_FILENAME) x $(ARG)

debug: .obj/$(polymorph_FILENAME)
	$(PLATFORM_PATH)\gdb --quiet --batch -ex run -ex bt full -ex quit --args .obj/$(polymorph_FILENAME)

dump: .obj/$(polymorph_FILENAME).dump

SHADERS=$($(CONFIG)_SHADERS)
SHADER_NAMES=vertex-shader.glsl geometry-shader.glsl fragment-shader.glsl
SHADER_DIRECTORY=$($(SHADERS)_SHADER_DIRECTORY)
SHADER_RESOURCES=$($(SHADERS)_SHADER_RESOURCES)
RESOURCES=polyhedron.ico $(SHADER_RESOURCES) src/polymorph.scr.manifest

full_SHADER_DIRECTORY=src
minified_SHADER_DIRECTORY=.obj/minified

$(foreach shaders,full minified,$(eval $(shaders)_SHADER_RESOURCES=$(foreach name,$(SHADER_NAMES),$$($(shaders)_SHADER_DIRECTORY)/$(name))))

$(minified_SHADER_DIRECTORY)/%.glsl: src/%.glsl minify.pl | $(minified_SHADER_DIRECTORY)
	c:\strawberry\perl\bin\perl minify.pl "$<" "$@"

.obj/$(PLATFORM)/$(CONFIG)/resources.o: $(RESOURCES)

.obj: ; -md .obj
.obj/minified: | .obj ; -md .obj\minified
$(foreach platform,$(PLATFORMS),$(eval .obj/$(platform): | .obj ; -md .obj\$(platform)))
$(foreach platform,$(PLATFORMS),$(foreach config,$(CONFIGS),$(eval .obj/$(platform)/$(config): | .obj/$(platform) ; -md .obj\$(platform)\$(config))))

include program.mak
include build-all.mak
