# -*- Makefile-gmake -*-

# Nested $(eval) requires GNU Make version 3.81 or later (Bug #1517).

DEPFLAGS=-MMD

name=$($(program)_FILENAME)
source_prefix=$($(program)_SOURCE_PREFIX)
extra_objects=$($(program)_EXTRA_OBJECTS)
objects=$(extra_objects) $(foreach object,$($(program)_OBJECTS),.obj/$(program)-$(object))
depends=$(foreach object,$($(program)_OBJECTS),.obj/$(program)-$(object:%.o=%.d))
cppflags=$($(program)_CPPFLAGS) $(CPPFLAGS)
ccflags=$(CFLAGS) $(CCFLAGS) $($(program)_CFLAGS) $($(program)_CCFLAGS)
cxxflags=$(CFLAGS) $(CXXFLAGS) $($(program)_CFLAGS) $($(program)_CXXFLAGS)
ldflags=$($(program)_LDFLAGS) $(LDFLAGS)
ldlibs=$($(program)_LDLIBS) $(LDLIBS)

define compile
.obj/$(program)-%.o: $(source_prefix)%.cpp | .obj
	$(CXX) $(cppflags) $(cxxflags) $(DEPFLAGS) $$< -c -o $$@
.obj/$(program)-%.o: $(source_prefix)%.c | .obj
	$(CC) $(cppflags) $(ccflags) $(DEPFLAGS) $$< -c -o $$@
ifdef $(program)_FILENAME
$(name): $(objects)
	$(CXX) $(cflags) $(cxxflags) $(ldflags) $(objects) $(ldlibs) -o $(name)
endif
endef

$(foreach program,$(PROGRAMS),$(eval $(compile)))

.obj:
	-mkdir .obj

.PHONY: clean
clean:
	-rmdir /s /q .obj
	-del $(EXTRA_CLEAN) $(foreach program,$(PROGRAMS),$(name))

.DELETE_ON_ERROR:

-include $(foreach program,$(PROGRAMS),$(depends))
