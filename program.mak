# -*- Makefile-gmake -*-

# Nested $(eval) requires GNU Make version 3.81 or later (Bug #1517).

DEPFLAGS=-MMD

name=$($(program)_FILENAME)
source_prefix=$($(program)_SOURCE_PREFIX)
extra_objects=$($(program)_EXTRA_OBJECTS)
objects=$(foreach object,$($(program)_OBJECTS),.obj/$(program)-$(object)) $(extra_objects)
depends=$(foreach object,$($(program)_OBJECTS),.obj/$(program)-$(object:%.o=%.d))
cppflags=$($(program)_CPPFLAGS) $(CPPFLAGS)
cflags=$($(program)_CFLAGS) $(CFLAGS)
ccflags=$($(program)_CCFLAGS) $(CCFLAGS)
cxxflags=$($(program)_CXXFLAGS) $(CXXFLAGS)
ldflags=$($(program)_LDFLAGS) $(LDFLAGS)
ldlibs=$($(program)_LDLIBS) $(LDLIBS)

define compile
.obj/$(program)-%.o: $(source_prefix)%.cpp | .obj
	$(CXX) $(cppflags) $(cflags) $(cxxflags) -fverbose-asm $$< -S -o $$(@:.o=.s)
	$(CXX) $(cppflags) $(cflags) $(cxxflags) $(DEPFLAGS) $$< -c -o $$@
.obj/$(program)-%.o: $(source_prefix)%.c | .obj
	$(CC) $(cppflags) $(cflags) $(ccflags) $(DEPFLAGS) $$< -c -o $$@
ifdef $(program)_FILENAME
$(name): $(objects)
	$(CXX) $(cflags) $(cxxflags) $(ldflags) $(objects) $(ldlibs) -o $(name)
endif
endef

$(foreach program,$(PROGRAMS),$(eval $(compile)))

.obj:
	-mkdir .obj

.PHONY: clean veryclean
clean:
	-rm -fv $(foreach program,$(PROGRAMS),$(objects))
	-rm -fv $(EXTRA_CLEAN)

veryclean:
	-rm -rfv .obj
	-rm -fv $(foreach program,$(PROGRAMS),$(name))
	-rm -fv $(EXTRA_CLEAN)

.DELETE_ON_ERROR:

-include $(foreach program,$(PROGRAMS),$(depends))
