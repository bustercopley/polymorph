# -*- Makefile-gmake -*-

# Nested $(eval) requires GNU Make version 3.81 or later (Bug #1517).

DEPFLAGS=-MMD

name=$($(program)_FILENAME)
source_prefix=$($(program)_SOURCE_PREFIX)
objdir=$($(program)_OBJDIR)
tmpdir=$(objdir)/tmp
tmpdir_wpath=$(subst /,\,$(tmpdir))
objects=$(foreach object,$($(program)_OBJECTS),$(objdir)/$(object))
depends=$(foreach object,$($(program)_OBJECTS),$(objdir)/$(object:%.o=%.d))
cppflags=$($(program)_CPPFLAGS) $(CPPFLAGS)
ccflags=$(CFLAGS) $(CCFLAGS) $($(program)_CFLAGS) $($(program)_CCFLAGS)
cxxflags=$(CFLAGS) $(CXXFLAGS) $($(program)_CFLAGS) $($(program)_CXXFLAGS)
resflags=$(cppflags) $($(program)_RESFLAGS)
ldflags=$($(program)_LDFLAGS) $(LDFLAGS)
ldlibs=$($(program)_LDLIBS) $(LDLIBS)

define compile
$(tmpdir): | $(objdir) ; -md $(tmpdir_wpath)
$(objdir)/%.o: $(source_prefix)%.cpp | $(objdir)
	$(CXX) $(DEPFLAGS) $(cppflags) $(cxxflags) $$< -c -o $$@
$(objdir)/%.o: $(source_prefix)%.c | $(objdir)
	$(CC) $(DEPFLAGS) $(cppflags) $(ccflags) $$< -c -o $$@
$(objdir)/%.o: $(source_prefix)%.rc | $(objdir) $(tmpdir)
	$(CC) $(cppflags) -MM -MT $$@ -MF $$(@:%.o=%.d) -x c $$<
	windres $(resflags) $$< $$@
ifdef $(program)_FILENAME
$(name) $(objdir)/$(name).s: $(objects) | $(tmpdir)
	SET TMP=$(tmpdir_wpath) && $(CXX) $(cflags) $(cxxflags) $(ldflags) $(objects) $(ldlibs) -o $(name) -save-temps=obj
	-copy /b $(tmpdir_wpath)\*.s $(subst /,\,$(objdir))\$(name).s
	-del $(tmpdir_wpath)\*.s
endif
endef

$(foreach program,$(PROGRAMS),$(eval $(compile)))

.PHONY: clean
clean:
	-rd /s /q .obj
	-del $(subst /,\,$(EXTRA_CLEAN))

.DELETE_ON_ERROR:

-include $(foreach program,$(PROGRAMS),$(depends))
