PROGRAMS=polymorph
PLATFORM=x64
CONFIG=tiny

PLATFORMS=x64 x86
CONFIGS=tiny base debug
$(if $(filter $(CONFIG),$(CONFIGS)),,$(error Bad config "$(CONFIG)"))
$(if $(filter $(PLATFORM),$(PLATFORMS)),,$(error Bad platform "$(PLATFORM)"))

OLDPATH:=$(PATH)
PATH=$(PLATFORM_PATH);$(OLDPATH)

CC=gcc
CXX=g++
CFLAGS=-pedantic -Wall -Wextra
CXXFLAGS=-std=c++1y

polymorph_FILENAME=$($(PLATFORM)_$(CONFIG)_APPNAME).scr
polymorph_OBJDIR=.obj/$(PLATFORM)/$(CONFIG)
polymorph_CPPFLAGS=$(CONFIG_CPPFLAGS)
polymorph_CFLAGS=$(CONFIG_CFLAGS)
polymorph_CXXFLAGS=$(CONFIG_CXXFLAGS)
polymorph_LDFLAGS=$(CONFIG_LDFLAGS)
polymorph_LDLIBS=$(CONFIG_LDLIBS)
polymorph_RESFLAGS=-DPLATFORM_CONFIG=$(PLATFORM)_$(CONFIG) -I$(SHADER_DIRECTORY)
polymorph_EXTRA_OBJECTS=.obj/$(PLATFORM)/$(CONFIG)/resources-res.o
polymorph_SOURCE_PREFIX=src/
polymorph_OBJECTS=\
arguments.o bump.o dialog.o glinit.o graphics.o kdtree.o main.o markov.o memory.o \
model.o partition.o polymorph.o random.o reposition.o rodrigues.o settings.o systems.o make_system.o

common_CPPFLAGS=-DUNICODE -D_UNICODE
common_CFLAGS=-msse3 -mfpmath=sse -fno-ident
common_CXXFLAGS=-fno-rtti -fno-exceptions -ffast-math
common_LDFLAGS=-mwindows -municode
common_LDLIBS=-lopengl32 -lcomctl32 -lshell32

base_CPPFLAGS=$(common_CPPFLAGS) -DENABLE_PRINT
base_CFLAGS=$(common_CFLAGS) -flto -Os
base_CXXFLAGS=$(common_CXXFLAGS)
base_LDFLAGS=$(common_LDFLAGS) -s
base_LDLIBS=$(common_LDLIBS)
base_SHADERS=minified

tiny_CPPFLAGS=$(base_CPPFLAGS) -DTINY
tiny_CFLAGS=$(base_CFLAGS) -fno-asynchronous-unwind-tables
tiny_CXXFLAGS=$(base_CXXFLAGS)
tiny_LDFLAGS=$(base_LDFLAGS) -nostdlib -Wl,--disable-runtime-pseudo-reloc --entry=$(PLATFORM_ENTRY_POINT)
tiny_LDLIBS=$(base_LDLIBS) -lgdi32 -ladvapi32 -luser32 -lkernel32
tiny_SHADERS=minified

debug_CPPFLAGS=$(common_CPPFLAGS) -DENABLE_PRINT
debug_CFLAGS=$(common_CFLAGS) -g -ggdb
debug_CXXFLAGS=$(common_CXXFLAGS)
debug_LDFLAGS=$(common_LDFLAGS)
debug_LDLIBS=$(common_LDLIBS)
debug_SHADERS=full

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
CONFIG_SHADERS=$($(CONFIG)_SHADERS)

EXTRA_CLEAN=Polymorph*.scr

.PHONY: all test debug

all: $(polymorph_FILENAME)

test: all
	$(polymorph_FILENAME)

testx: all
	$(polymorph_FILENAME) x

debug: all
	$(PLATFORM_PATH)\gdb --quiet --batch -ex run -ex bt full -ex quit --args $(polymorph_FILENAME)

SHADER_NAMES=vertex-shader.glsl geometry-shader.glsl fragment-shader.glsl
SHADER_DIRECTORY=$($(CONFIG_SHADERS)_SHADER_DIRECTORY)
SHADER_RESOURCES=$($(CONFIG_SHADERS)_SHADER_RESOURCES)
RESOURCES=$(SHADER_RESOURCES) src/polymorph.scr.manifest

full_SHADER_DIRECTORY=src
minified_SHADER_DIRECTORY=.obj/minified

$(foreach shaders,full minified,$(eval $(shaders)_SHADER_RESOURCES=$(foreach name,$(SHADER_NAMES),$$($(shaders)_SHADER_DIRECTORY)/$(name))))

$(minified_SHADER_DIRECTORY)/%.glsl: src/%.glsl minify.pl | $(minified_SHADER_DIRECTORY)
	c:\strawberry\perl\bin\perl minify.pl "$<" "$@"

.obj/$(PLATFORM)/$(CONFIG)/resources-res.o: $(RESOURCES) src/resources.h

.obj: ; -md .obj
.obj/minified: | .obj ; -md .obj\minified
$(foreach platform,$(PLATFORMS),$(eval .obj/$(platform): | .obj ; -md .obj\$(platform)))
$(foreach platform,$(PLATFORMS),$(foreach config,$(CONFIGS),$(eval .obj/$(platform)/$(config): | .obj/$(platform) ; -md .obj\$(platform)\$(config))))

include program.mak
include build-all.mak
