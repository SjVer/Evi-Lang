STDLIB_CC = clang
AR = ar
ARFLAGS = -rc

STDLIB_CXXFLAGS = -D COMPILER=\"$(STDLIB_CC)\" -D LLVM_VERSION=$(LLVM_VERSION)

STDLIB_SRCDIR = $(STDLIB_SRC_DIR)/src
STDLIB_EXT = .evi.c
STDLIB_OBJDIR = $(BINDIR)/obj/stdlib
STDLIB_LIB = $(BINDIR)/libevi.a

STDLIB_SRC = $(wildcard $(STDLIB_SRCDIR)/**/*$(STDLIB_EXT))
STDLIB_OBJ = $(STDLIB_SRC:$(STDLIB_SRCDIR)/%$(STDLIB_EXT)=$(STDLIB_OBJDIR)/%.o)

STDLIB_OBJCOUNT_NOPAD = $(shell v=`echo $(STDLIB_OBJ) | wc -w`; echo `seq 1 $$(expr $$v)`)
STDLIB_OBJCOUNT = $(foreach v,$(STDLIB_OBJCOUNT_NOPAD),$(shell printf '%02d' $(v)))

.PRECIOUS: $(STDLIB_OBJ)
SHELL := /bin/bash

stdlib: $(STDLIB_OBJ) | makedirs
	@$(AR) $(ARFLAGS) -c $(STDLIB_LIB) $(STDLIB_OBJ)

$(STDLIB_OBJDIR)/%.o: $(STDLIB_SRCDIR)/%$(STDLIB_EXT) | makedirs
	@mkdir -p $(dir $@)
	@printf "[$(word 1,$(STDLIB_OBJCOUNT))/$(words $(STDLIB_OBJ))] compiling $(notdir $<) into $(notdir $@)..."
	@$(STDLIB_CC) $(STDLIB_CXXFLAGS) -I$(STDLIB_SRCDIR) -o $@ -c $< -fmodule-name=$($<:$(STDLIB_EXT)=.evi)
	@printf "\b\b done!\n"
	$(eval STDLIB_OBJCOUNT = $(filter-out $(word 1,$(STDLIB_OBJCOUNT)),$(STDLIB_OBJCOUNT)))
	@([ "$(word $(words $(STDLIB_OBJ)), $(STDLIB_OBJ))" == "$@" ] && echo "[stdlib] standard library compiled!") || true

.PHONY: clean
clean:
	@rm -rf $(STDLIB_OBJDIR)

.PHONY: makedirs
makedirs:
	@mkdir -p $(STDLIB_OBJDIR)