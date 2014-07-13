# -*- Makefile-gmake -*-

clean-all: clean
	$(MAKE) all-platforms-all-configs
all-platforms-all-configs: x64-all-configs x86-all-configs
x64-all-configs: | .obj/polymorph
	$(MAKE) PLATFORM=x64 all-configs
x86-all-configs: | .obj/polymorph
	$(MAKE) PLATFORM=x86 all-configs
all-configs: base tiny
data: | .obj/polymorph/$(PLATFORM)
	$(MAKE) .obj/polymorph/$(PLATFORM)/data
base: data
	$(MAKE) CONFIG=base all
tiny: data
	$(MAKE) CONFIG=tiny all

.PHONY: all-platforms-all-configs
.PHONY: x86-all-configs x64-all-configs
.PHONY: all-configs data base tiny
