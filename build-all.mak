# -*- Makefile-gmake -*-

dist: clean-all
	copy /y Polymorph.scr "g:\www\htdocs\polymorph\Polymorph.scr"
	copy /y Polymorph-x86.scr "g:\www\htdocs\polymorph\Polymorph (x86).scr"
clean-all: clean
	$(MAKE) all-platforms-all-configs
all-platforms-all-configs: x64-all-configs x86-all-configs
x64-all-configs: $(minified_SHADER_RESOURCES) | .obj/x64
	$(MAKE) PLATFORM=x64 all-configs
x86-all-configs: $(minified_SHADER_RESOURCES) | .obj/x86
	$(MAKE) PLATFORM=x86 all-configs
all-configs: base tiny
base:
	$(MAKE) CONFIG=base all
tiny:
	$(MAKE) CONFIG=tiny all

.PHONY: dist clean-all all-platforms-all-configs
.PHONY: x86-all-configs x64-all-configs
.PHONY: all-configs all-shaders base tiny
