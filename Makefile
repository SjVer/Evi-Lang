########################################################################
####################### Makefile Template ##############################
########################################################################

# Compiler settings - Can be customized.
CC = g++
LLVMVERSION = 12

MUTE = -Wall -Wno-varargs -Wno-write-strings -Wno-sign-compare -Wno-unused-function -Wno-init-list-lifetime
LLVMFLAGS = llvm-config-$(LLVMVERSION) --cxxflags
CXXFLAGS = $(MUTE) -lm -DCOMPILER=\"$(CC)\" `$(LLVMFLAGS)`
LDFLAGS = `$(LLVMFLAGS) --ldflags --system-libs --libs`

# Makefile settings - Can be customized.
APPNAME = evi
EXT = .cpp
SRCDIR = src
HEADERDIR = include
BINDIR = bin
OBJDIR = $(BINDIR)/obj

############## Do not change anything from here downwards! #############
SRC = $(wildcard $(SRCDIR)/*$(EXT))
# HEADERS = $(wildcard $(HEADERDIR)/*.h) $(wildcard $(HEADERDIR)/*.hpp)
OBJ = $(SRC:$(SRCDIR)/%$(EXT)=$(OBJDIR)/%.o)
APP = $(BINDIR)/$(APPNAME)
DEP = $(OBJ:$(OBJDIR)/%.o=%.d)

PHCS = phc.h

DEBUGDEFS = -D DEBUG -g

OBJCOUNT_NOPAD = $(shell v=`echo $(OBJ) | wc -w`; echo `seq 1 $$(expr $$v)`)
OBJCOUNT = $(foreach v,$(OBJCOUNT_NOPAD),$(shell printf '%02d' $(v)))


# UNIX-based OS variables & settings
RM = rm
MKDIR = mkdir
DELOBJ = $(OBJ)

########################################################################
####################### Targets beginning here #########################
########################################################################

.MAIN: $(APP)
all: $(APP)

# Builds the app
$(APP): $(OBJ) | makedirs
	@printf "[final] compiling final product $(notdir $@)..."
	@$(CC) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@printf "\b\b done!\n"

# Building rule for .o files and its .c/.cpp in combination with all .h
# $(OBJDIR)/%.o: $(SRCDIR)/%$(EXT) | makedirs
$(OBJDIR)/%.o: $(SRCDIR)/%$(EXT) | makedirs
	@printf "[$(word 1,$(OBJCOUNT))/$(words $(OBJ))] compiling $(notdir $<) into $(notdir $@)..."
	@$(CC) $(CXXFLAGS) -include $(PHCS) -I $(HEADERDIR) -o $@ -c $<
	@printf "\b\b done!\n"
	$(eval OBJCOUNT = $(filter-out $(word 1,$(OBJCOUNT)),$(OBJCOUNT)))

# Builds phc's
phcs: $(PHCS)
$(PHCS):
	@printf "[phc's] compiling $@..."
	@$(CC) $(CXXFLAGS) $(HEADERDIR)/$@ -c
	@printf "\b\b done!\n"


################### Cleaning rules for Unix-based OS ###################
# Cleans complete project
.PHONY: clean
clean:
	@# $(RM) $(DELOBJ) $(DEP) $(APP)
	@$(RM) -rf $(OBJDIR)
	@$(RM) -rf $(BINDIR)

.PHONY: test
test: $(APP)
	@printf "============ Running \"$(APP) test/test.evi -o bin/test.ll\" ============\n\n"
	@$(APP) test/test.evi -o bin/test.ll

.PHONY: test-debug
test-debug: debug $(APP)
	@printf "============ Running \"valgrind $(APP) test/test.evi -o bin/test.ll\" ============\n\n"
	@valgrind $(APP) test/test.evi -o bin/test.ll $(args)

.PHONY: routine
routine: $(APP) run clean

.PHONY: makedirs
makedirs:
	@$(MKDIR) -p $(BINDIR)
	@$(MKDIR) -p $(OBJDIR)

.PHONY: remake
remake: clean $(APP)

.PHONY: printdebug
printdebug:
	@echo "debug mode set!"

# .PHONY: debug
debug: CXXFLAGS += $(DEBUGDEFS)
debug: printdebug
debug: all


git:
	git add --all
# 	git status
	git commit -m 'upload'
	git push origin main

newfile:
	@test $(name) || ( echo "basename not given! ('make newfile name=BASENAME')"; false )
	touch $(SRCDIR)/$(name).cpp
	touch $(HEADERDIR)/$(name).hpp