# -*- Makefile-gmake -*-

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

.PHONY: clean-all all-platforms-all-configs
