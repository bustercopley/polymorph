# -*- Makefile-gmake -*-

clean-all: clean
	$(MAKE) all-platforms-all-configs
all-platforms-all-configs: x64-all-configs x86-all-configs
x64-all-configs: | .obj/polymorph
	$(MAKE) PLATFORM=x64 all-configs
x86-all-configs: | .obj/polymorph
	$(MAKE) PLATFORM=x86 all-configs
all-configs: base tiny
base:
	$(MAKE) CONFIG=base all
tiny:
	$(MAKE) CONFIG=tiny all

.PHONY: all-platforms-all-configs
.PHONY: x86-all-configs x64-all-configs
.PHONY: all-configs data base tiny
