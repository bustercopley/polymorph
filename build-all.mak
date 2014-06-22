# -*- Makefile-gmake -*-

clean-all: clean
	$(MAKE) all-platforms-all-configs
all-platforms-all-configs: x64-all-configs x86-all-configs
x64-all-configs: | .obj/polymorph
	$(MAKE) PLATFORM=x64 all-configs
x86-all-configs: | .obj/polymorph
	$(MAKE) PLATFORM=x86 all-configs
all-configs: base tiny
resources: | .obj/polymorph/$(PLATFORM)
	$(MAKE) .obj/polymorph/$(PLATFORM)/resources-res.o
base: resources
	$(MAKE) CONFIG=base all
tiny: resources
	$(MAKE) CONFIG=tiny all

.PHONY: all-platforms-all-configs
.PHONY: x86-all-configs x64-all-configs
.PHONY: all-configs resources
.PHONY: base tiny
