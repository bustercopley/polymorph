# Copyright 2012-2019 Richard Copley
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

# Available platforms and configs.
PLATFORMS=x64 x86
CONFIGS=tiny base debug

# Default platform and config (can be overridden on the Make command line).
PLATFORM=x64
CONFIG=tiny

# Top-level targets.
all: $(PLATFORM)-$(CONFIG)
test: $(PLATFORM)-$(CONFIG)-test
debug: $(PLATFORM)-$(CONFIG)-debug
clean: ; -rd /s /q .obj
.PHONY: all test debug clean

# Options common to all platforms and configs.
SRCDIR=src
CPPFLAGS=-DUNICODE
CFLAGS=-g -march=core2 -mtune=generic -mfpmath=sse -mno-stackrealign \
-fno-ident -ffast-math -Wall -Wextra -Werror
CXXFLAGS=-std=c++1z
LDFLAGS=-municode
LDLIBS=-lopengl32 -lcomctl32 -lshell32
RESOURCES=polyhedron.ico $(SRCDIR)/polymorph.scr.manifest
SHADER_NAMES=vertex-shader.glsl geometry-shader.glsl fragment-shader.glsl
OBJECTS=\
arguments.o bump.o dialog.o glinit.o graphics.o main.o markov.o memory.o \
model.o partition.o polymorph.o random.o reposition.o resources.o rodrigues.o \
settings.o systems.o make_system.o
DEBUGGER_ARGS=--batch --quiet -ex run -ex "bt full" -ex quit
PERL=perl.exe

# Config-specific options.
tiny_CPPFLAGS=-DTINY
tiny_CFLAGS=-flto -Os -fno-asynchronous-unwind-tables
tiny_CXXFLAGS=-fno-rtti -fno-exceptions
tiny_LDFLAGS=-nostdlib -Wl,--disable-runtime-pseudo-reloc -Wl,-e$(entry_point)
tiny_LDLIBS=-mwindows -lgdi32 -ladvapi32 -luser32 -lkernel32
tiny_SHADERS=minified

base_CPPFLAGS=-DENABLE_PRINT -DENABLE_GLCHECK -DENABLE_GLDEBUG
base_CFLAGS=-flto -Os
base_CXXFLAGS=
base_LDFLAGS=
base_LDLIBS=-mconsole -lgdi32
base_SHADERS=full

debug_CPPFLAGS=-DENABLE_PRINT -DENABLE_GLCHECK -DENABLE_GLDEBUG
debug_CFLAGS=-O0 -ggdb3
debug_CXXFLAGS=
debug_LDFLAGS=
debug_LDLIBS=-mconsole -lgdi32
debug_SHADERS=full

# Shader-specific options.
full_SHADERS_DIR=$(SRCDIR)
minified_SHADERS_DIR=.obj/minified

$(foreach shaders,full minified,\
$(eval $(shaders)_RESOURCES=$(SHADER_NAMES:%=$($(shaders)_SHADERS_DIR)/%)))

# Platform-specific options.
x86_TOOLDIR=C:\msys64\mingw32\bin
x64_TOOLDIR=C:\msys64\mingw64\bin
x86_ENTRY_POINT=_RawEntryPoint@0
x64_ENTRY_POINT=RawEntryPoint

# Options that depend on both platform and config.
x64_base_APPNAME=Polymorph-base
x86_base_APPNAME=Polymorph-x86-base
x64_tiny_APPNAME=Polymorph
x86_tiny_APPNAME=Polymorph-x86
x64_debug_APPNAME=Polymorph-debug
x86_debug_APPNAME=Polymorph-x86-debug

# Internal variables.
filename=$($(platform)_$(config)_APPNAME).scr
objdir=.obj/$(platform)/$(config)
shaders=$($(config)_SHADERS)
shaders_dir=$($(shaders)_SHADERS_DIR)
tooldir=$($(platform)_TOOLDIR)
entry_point=$($(platform)_ENTRY_POINT)
objects=$(OBJECTS:%=$(objdir)/%)
cppflags=$(CPPFLAGS) $($(config)_CPPFLAGS)
cflags=$(CFLAGS) $($(config)_CFLAGS)
cxxflags=$(cflags) $(CXXFLAGS) $($(config)_CXXFLAGS)
ldflags=$(LDFLAGS) $($(config)_LDFLAGS)
ldlibs=$(LDLIBS) $($(config)_LDLIBS)
resflags=-DPLATFORM_CONFIG=$(platform)_$(config) -I$(shaders_dir)
resources=$(RESOURCES) $($(shaders)_RESOURCES)

# Rules that depend on platform and config.
define platform-config-defs

# Top-level targets (advanced).
$(platform)-$(config): $(filename)

$(platform)-$(config)-test: $(platform)-$(config)
	$(filename) $(ARGS)

$(platform)-$(config)-debug: $(objdir)/$(filename)
	gdb $(DEBUGGER_ARGS) --args $(objdir)/$(filename) $(ARGS)

clean: $(platform)-$(config)-clean
$(platform)-$(config)-clean:
	-del $(filename)

all-platforms-all-configs: $(platform)-$(config)
$(platform)-all-configs: $(platform)-$(config)
all-platforms-$(config): $(platform)-$(config)

.PHONY: $(platform)-$(config)
.PHONY: $(platform)-$(config)-test
.PHONY: $(platform)-$(config)-debug
.PHONY: $(platform)-$(config)-clean
.PHONY: $(platform)-all-configs
.PHONY: all-platforms-$(config)
.PHONY: all-platforms-all-configs

# Select platform toolchain by giving PATH a target-specific variable value.
$(platform)-$(config)-%: PATH=$(tooldir)
$(filename): PATH=$(tooldir)
$(objdir)/%: PATH=$(tooldir)

# Bottom-level targets.
$(filename): $(objdir)/$(filename)
	strip $$< -o $$@

$(objdir)/$(filename): $(objects)
	$(CXX) $(cxxflags) $(ldflags) $$^ $(ldlibs) -o $$@

$(objdir)/%.o: $(SRCDIR)/%.cpp | $(objdir)
	$(CXX) -c -o $$@ $$< -MMD -MP $(cppflags) $(cxxflags)

$(objdir)/%.o: $(SRCDIR)/%.rc | $(objdir)
	$(CC) -x c $$< $(cppflags) -MM -MP -MT $$@ -MF $$(@:%.o=%.d)
	windres $(cppflags) $(resflags) $$< $$@

$(objdir)/resources.o: $(resources)
.obj/$(platform)/$(config): | .obj/$(platform) ; -md "$$@"

# Automatic prerequisites.
-include $(objects:%.o=%.d)

endef

# Invoke platform-config-defs for each platform and each config.
define platform-defs
$(foreach config,$(CONFIGS),$(eval $(platform-config-defs)))
.obj/$(platform): | .obj ; -md "$$@"
endef
$(foreach platform,$(PLATFORMS),$(eval $(platform-defs)))

# Miscellaneous.
.obj: ; -md "$@"
.obj/minified: | .obj ; -md "$@"
.obj/minified/%.glsl: $(SRCDIR)/%.glsl minify.pl | .obj/minified
	$(PERL) minify.pl "$<" "$@"
