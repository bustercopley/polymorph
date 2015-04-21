# -*- Makefile-gmake -*-

dist: clean-all
	copy /y Polymorph.scr "g:\www\htdocs\polymorph\Polymorph.scr"
	copy /y Polymorph-x86.scr "g:\www\htdocs\polymorph\Polymorph (x86).scr"
clean-all: clean
	$(MAKE) all-platforms-all-configs

all-platforms-all-configs: $(foreach platform,$(PLATFORMS),$(platform)-all-configs)

define platform-targets
$(platform)-all-configs: $(foreach config,$(CONFIGS),$(platform)-$(config))
.PHONY: $(platform)-all-configs
endef

define config-targets
all-platforms-$(config): $(foreach platform,$(PLATFORMS),$(platform)-$(config))
.PHONY: all-platforms-$(config)
endef

define platform-config-targets
$(platform)-$(config): $($($(config)_SHADERS)_SHADER_RESOURCES) | .obj/$(platform)
	$(MAKE) PLATFORM=$(platform) CONFIG=$(config) all
.PHONY: $(platform)-$(config)
endef

$(foreach config,$(CONFIGS),$(eval $(config-targets)))
$(foreach platform,$(PLATFORMS),$(eval $(platform-targets)))
$(foreach platform,$(PLATFORMS),$(foreach config,$(CONFIGS),$(eval $(platform-config-targets))))

.PHONY: dist clean-all all-platforms-all-configs
