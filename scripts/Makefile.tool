#
# toolprogs-y := example
# Will compile example.c and create an executable named example
#
# toolprogs-y := tool1
# tool1-y := main.o helper.o
#
# Per-program link inputs, like Makefile.test:
#   LIBS_tool1    := $(objtree)/net/built-in.o -lpcap
#   LDFLAGS_tool1 := -static

__toolprogs := $(sort $(toolprogs-y))

# Executables compiled from a single .c file
tool-csingle := $(foreach m,$(__toolprogs),$(if $($(m)-y),,$(m)))

# Executables linked based on several .o files
tool-cmulti := $(foreach m,$(__toolprogs), $(if $($(m)-y),$(m)))

# Object (.o) files compiled from .c files
tool-cobjs := $(sort $(foreach m,$(__toolprogs),$($(m)-y)))

__obj_fixed := $(patsubst %/,%,$(obj))
__src_fixed := $(patsubst %/,%,$(src))

# Add $(obj) prefix to all paths
tool-csingle := $(addprefix $(__obj_fixed)/,$(tool-csingle))
tool-cmulti  := $(addprefix $(__obj_fixed)/,$(tool-cmulti))
tool-cobjs   := $(addprefix $(__obj_fixed)/,$(tool-cobjs))

# Options to toolcc. KBUILD_SUBDIR_CCFLAGS carries the header roots the srctree
# sets up (-I$(srctree), -I$(srctree)/hpc, ...), which tool sources need as much
# as target objects do.
toolc_flags = -Wp,-MD,$(depfile) $(KBUILD_CFLAGS) $(KBUILD_CPPFLAGS) \
	      $(KBUILD_SUBDIR_CCFLAGS) \
	      $(USERINCLUDE) $(tool_CFLAGS) -I. -I../ \
	      -include $(srctree)/arch/os/$(PLATFORM)/platform.h

toolld_flags =
toolld_builtin =
toolld_libs = $(KBUILD_LIBS)

# tool-csingle -> executable
quiet_cmd_tool-csingle = CC      $@
      cmd_tool-csingle = $(CC) $(LDFLAGS_$(*F)) $(toolc_flags) -o $@ $< \
                               $(toolld_builtin) $(toolld_libs) $(LIBS_$(*F))

$(tool-csingle): $(__obj_fixed)/%: $(__src_fixed)/%.c FORCE
	$(call if_changed_dep,tool-csingle)

# tool-cobjs -> .o
quiet_cmd_tool-cobjs	= CC      $@
      cmd_tool-cobjs	= $(CC) $(EXE_LDFLAGS) $(toolc_flags) -c -o $@ $<

$(tool-cobjs): $(__obj_fixed)/%.o: $(__src_fixed)/%.c FORCE
	$(call if_changed_dep,tool-cobjs)

# Link an executable based on list of .o files
quiet_cmd_tool-cmulti	= LD      $@
      cmd_tool-cmulti	= $(CC) $(LDFLAGS_$(*F)) $(EXE_LDFLAGS) \
			  $(toolld_flags) -o $@ \
			  $(addprefix $(obj)/,$($(@F)-y)) $(toolld_builtin) \
      			  $(toolld_libs) $(LIBS_$(*F))
$(tool-cmulti): $(__obj_fixed)/%: $(tool-cobjs) FORCE
	$(call if_changed,tool-cmulti)

# A tool that links an archive (LIBS_<prog> += .../built-in.o) has to be
# relinked when that archive changes, not only when its own objects do —
# otherwise a rebuilt library silently leaves a stale executable behind.
$(foreach m,$(__toolprogs), \
	$(eval $(__obj_fixed)/$(m): $(filter %.o %.a,$(LIBS_$(m)))))

# clean support
targets += $(tool-csingle) $(tool-cmulti) $(tool-cobjs)
always += $(tool-csingle) $(tool-cmulti)
