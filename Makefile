# Copyright 2016 Richard Copley
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

SHELL=cmd

# Override on the command line, e.g., "make PLATFORM=x86 CONFIG=base".
PLATFORM=x64
CONFIG=tiny

PLATFORMS=x64 x86
CONFIGS=tiny base debug

$(if $(filter $(CONFIG),$(CONFIGS)),,$(error Bad config "$(CONFIG)"))
$(if $(filter $(PLATFORM),$(PLATFORMS)),,$(error Bad platform "$(PLATFORM)"))

x86_TOOLDIR=C:\msys64\mingw32\bin
x64_TOOLDIR=C:\msys64\mingw64\bin
PATH=$($(PLATFORM)_TOOLDIR)

x64_base_APPNAME=Polymorph-base
x86_base_APPNAME=Polymorph-x86-base
x64_tiny_APPNAME=Polymorph
x86_tiny_APPNAME=Polymorph-x86
x64_debug_APPNAME=Polymorph-debug
x86_debug_APPNAME=Polymorph-x86-debug

FILENAME=$($(PLATFORM)_$(CONFIG)_APPNAME).scr
OBJECTS=\
arguments.o bump.o dialog.o glinit.o graphics.o main.o markov.o memory.o \
model.o partition.o polymorph.o random.o reposition.o resources.o rodrigues.o \
settings.o systems.o make_system.o

OBJDIR=.obj/$(PLATFORM)/$(CONFIG)
SRCDIR=src
ENTRY_POINT=_custom_startup
EXTRA_CLEAN=Polymorph*.scr

CPPFLAGS=-DUNICODE
CFLAGS=-g -march=core2 -mtune=generic -mfpmath=sse -mno-stackrealign -fno-ident -ffast-math -pedantic -Wall -Wextra
CXXFLAGS=-std=c++1z
LDFLAGS=-municode
LDLIBS=-lopengl32 -lcomctl32 -lshell32
RESFLAGS=-DPLATFORM_CONFIG=$(PLATFORM)_$(CONFIG) -I$(SHADER_DIRECTORY)

tiny_CPPFLAGS=-DTINY
tiny_CFLAGS=-flto -Os -fno-asynchronous-unwind-tables
tiny_CXXFLAGS=-fno-rtti -fno-exceptions
tiny_LDFLAGS=-nostdlib -Wl,--disable-runtime-pseudo-reloc -Wl,-e$(ENTRY_POINT)
tiny_LDLIBS=-mwindows -lgdi32 -ladvapi32 -luser32 -lkernel32
tiny_SHADERS=minified

base_CPPFLAGS=-DENABLE_PRINT -DENABLE_GLCHECK -DENABLE_GLDEBUG
base_CFLAGS=-flto -Os
base_CXXFLAGS=
base_LDFLAGS=
base_LDLIBS=-mconsole -lgdi32
base_SHADERS=full

debug_CPPFLAGS=-DENABLE_PRINT -DENABLE_GLCHECK -DENABLE_GLDEBUG
debug_CFLAGS=-Og
debug_CXXFLAGS=
debug_LDFLAGS=
debug_LDLIBS=-mconsole -lgdi32
debug_SHADERS=full

full_SHADER_DIRECTORY=src
minified_SHADER_DIRECTORY=.obj/minified

SHADERS=$($(CONFIG)_SHADERS)
SHADER_NAMES=vertex-shader.glsl geometry-shader.glsl fragment-shader.glsl
SHADER_DIRECTORY=$($(SHADERS)_SHADER_DIRECTORY)
SHADER_RESOURCES=$($(SHADERS)_SHADER_RESOURCES)
$(foreach shaders,full minified,$(eval $(shaders)_SHADER_RESOURCES=$(SHADER_NAMES:%=$$($(shaders)_SHADER_DIRECTORY)/%)))

RESOURCES=polyhedron.ico $(SHADER_RESOURCES) src/polymorph.scr.manifest

objects=$(OBJECTS:%=$(OBJDIR)/%)
cppflags=$(CPPFLAGS) $($(CONFIG)_CPPFLAGS)
cflags=$(CFLAGS) $($(CONFIG)_CFLAGS)
cxxflags=$(CXXFLAGS) $($(CONFIG)_CXXFLAGS)
ldflags=$(LDFLAGS) $($(CONFIG)_LDFLAGS)
ldlibs=$(LDLIBS) $($(CONFIG)_LDLIBS)
resflags=$(RESFLAGS)

all: $(FILENAME) dump

test: all
	$(FILENAME) $(ARG)

debug: .obj/$(FILENAME)
	gdb --quiet --batch -ex run -ex bt full -ex quit --args .obj/$(FILENAME)

dump: .obj/$(FILENAME).dump

clean:
	-rd /s /q .obj
	-del $(EXTRA_CLEAN:%="%")

.PHONY: all test debug dump clean

$(FILENAME): .obj/$(FILENAME)
	strip .obj/$(FILENAME) -o $(FILENAME)

.obj/$(FILENAME).dump: .obj/$(FILENAME)
	>.obj\$(FILENAME).dump ( objdump -Mintel --no-show-raw-insn -d -C -l .obj/$(FILENAME) & objdump -s -j.data .obj/$(FILENAME) )

.obj/$(PLATFORM)/$(CONFIG)/resources.o: $(RESOURCES)

.obj/$(FILENAME): $(objects)
	$(CXX) $(cflags) $(cxxflags) $(ldflags) $(objects) $(ldlibs) -o .obj/$(FILENAME)
	-@size .obj/$(FILENAME)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) -c $< -o $@ -MMD $(cppflags) $(cflags) $(cxxflags)
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) -c $< -o $@ -MMD $(cppflags) $(cflags) $(ccflags)
$(OBJDIR)/%.o: $(SRCDIR)/%.S | $(OBJDIR)
	$(CC) -c $< -o $@ -MMD
$(OBJDIR)/%.o: $(SRCDIR)/%.rc | $(OBJDIR)
	$(CC) -x c $< $(cppflags) -MM -MT $@ -MF $(@:%.o=%.d)
	windres $(cppflags) $(resflags) $< $@

$(minified_SHADER_DIRECTORY)/%.glsl: src/%.glsl minify.pl | $(minified_SHADER_DIRECTORY)
	c:\strawberry\perl\bin\perl.exe minify.pl "$<" "$@"

.obj: ; -md "$@"
.obj/minified $(PLATFORMS:%=.obj/%): | .obj ; -md "$@"
$(foreach platform,$(PLATFORMS),$(eval $(CONFIG:%=.obj/$(platform)/%): | .obj/$(platform) ; -md "$$@"))

-include $(OBJECTS:%.o=$(OBJDIR)/%.d)
include build-all.mak
