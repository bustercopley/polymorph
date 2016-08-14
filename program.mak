# -*- Makefile-gmake -*-

# Nested $(eval) requires GNU Make version 3.81 or later (Bug #1517).

name=$($(program)_FILENAME)
source_prefix=$($(program)_SOURCE_PREFIX)
objdir=$($(program)_OBJDIR)
objdir_wpath=$(subst /,\,$(objdir))
objects=$(foreach object,$($(program)_OBJECTS),$(objdir)/$(object))
depends=$(foreach object,$($(program)_OBJECTS),$(objdir)/$(object:%.o=%.d))
cppflags=$($(program)_CPPFLAGS) $(CPPFLAGS)
cflags=$(CFLAGS) $($(program)_CFLAGS)
ccflags=$(cflags) $(CCFLAGS) $($(program)_CCFLAGS)
cxxflags=$(cflags) $(CXXFLAGS) $($(program)_CXXFLAGS)
resflags=$(cppflags) $($(program)_RESFLAGS)
ldflags=$($(program)_LDFLAGS) $(LDFLAGS)
ldlibs=$($(program)_LDLIBS) $(LDLIBS)

define compile
$(objdir)/%.o: $(source_prefix)%.cpp | $(objdir)
	$(CXX) -MMD $(cppflags) $(cxxflags) $$< -c -o $$@
$(objdir)/%.o: $(source_prefix)%.c | $(objdir)
	$(CC) -MMD $(cppflags) $(ccflags) $$< -c -o $$@
$(objdir)/%.o: $(source_prefix)%.rc | $(objdir)
	$(CC) $(cppflags) -MM -MT $$@ -MF $$(@:%.o=%.d) -x c $$<
	windres $(resflags) $$< $$@
ifdef $(program)_FILENAME
.obj/$(name): $(objects)
	$(CXX) $(cxxflags) $(ldflags) $(objects) $(ldlibs) -o .obj/$(name)
	-@size .obj/$(name)
.obj/$(name).dump: .obj/$(name)
	>.obj\$(name).dump ( objdump -Mintel -d -C -l .obj/$(name) & objdump -s -j.data .obj/$(name) | objdumpfloats.exe )
$(name): .obj/$(name)
	-@copy .obj\$(name)
	strip $(name)
endif
endef

$(foreach program,$(PROGRAMS),$(eval $(compile)))

.PHONY: clean
clean:
	-rd /s /q .obj
	-del $(subst /,\,$(EXTRA_CLEAN))

.DELETE_ON_ERROR:

-include $(foreach program,$(PROGRAMS),$(depends))
