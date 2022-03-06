########################################################################
####################### Makefile Template ##############################
########################################################################

# Compiler settings - Can be customized.
CC = clang++
LLVMVERSION = 12
CC_PATH = /usr/bin/clang
LD_PATH = /usr/bin/ld
# STATICLIB_DIR = /usr/lib/
STATICLIB_DIR = $(PWD)/bin/
# STDLIB_DIR = /usr/share/evi/
STDLIB_DIR = $(PWD)/stdlib/headers/
TARGET = x86_64-linux-gnu

MUTE = -Wall -Wno-varargs -Wno-write-strings -Wno-sign-compare -Wno-unused-function -Wno-comment
LLVMFLAGS = llvm-config-$(LLVMVERSION) --cxxflags
DEFS = COMPILER=\"$(CC)\" LD_PATH=\"$(LD_PATH)\" STATICLIB_DIR=\"$(STATICLIB_DIR)\" STDLIB_DIR=\"$(STDLIB_DIR)\"
CXXFLAGS = $(MUTE) $(addprefix -D,$(DEFS)) `$(LLVMFLAGS)`
LDFLAGS = `$(LLVMFLAGS) --ldflags --system-libs --libs`

# Makefile settings - Can be customized.
APPNAME = evi
EXT = .cpp
SRCDIR = src
HEADERDIR = include
BINDIR = bin
OBJDIR = $(BINDIR)/obj

STDLIB_SRC_DIR = stdlib
export LLVMVERSION BINDIR STDLIB_SRC_DIR

############## Do not change anything from here downwards! #############
SRC = $(wildcard $(SRCDIR)/*$(EXT))
# HEADERS = $(wildcard $(HEADERDIR)/*.h) $(wildcard $(HEADERDIR)/*.hpp)
OBJ = $(SRC:$(SRCDIR)/%$(EXT)=$(OBJDIR)/%.o)
APP = $(BINDIR)/$(APPNAME)
DEP = $(OBJ:$(OBJDIR)/%.o=%.d)

PHC = $(HEADERDIR)/pch.h
PHCFLAGS = $(CXXFLAGS) -x c++-header $(PHC)
# INC_PHC_FLAG = -include $(PHC)
INC_PHC_FLAG = -include-pch $(PHC).gch

DEBUGDEFS = -DDEBUG -ggdb

OBJCOUNT_NOPAD = $(shell v=`echo $(OBJ) | wc -w`; echo `seq 1 $$(expr $$v)`)
OBJCOUNT = $(foreach v,$(OBJCOUNT_NOPAD),$(shell printf '%02d' $(v)))


# UNIX-based OS variables & settings
RM = rm
MKDIR = mkdir
DELOBJ = $(OBJ)
SHELL := /bin/bash

########################################################################
####################### Targets beginning here #########################
########################################################################

.MAIN: $(APP)
all: $(APP) stdlib

# Builds the app
$(APP): $(OBJ) | makedirs
	@printf "[final] compiling final product $(notdir $@)..."
	@$(CC) $(CXXFLAGS) -I$(HEADERDIR)/$(TARGET) -o $@ $^ $(LDFLAGS)
	@printf "\b\b done!\n"

# Building rule for .o files and its .c/.cpp in combination with all .h
# $(OBJDIR)/%.o: $(SRCDIR)/%$(EXT) | makedirs
$(OBJDIR)/%.o: $(SRCDIR)/%$(EXT) | makedirs
	@printf "[$(word 1,$(OBJCOUNT))/$(words $(OBJ))] compiling $(notdir $<) into $(notdir $@)..."
	@$(CC) $(CXXFLAGS) -I$(HEADERDIR)/$(TARGET) $(INC_PHC_FLAG) -I $(HEADERDIR) -o $@ -c $<
	@printf "\b\b done!\n"
	$(eval OBJCOUNT = $(filter-out $(word 1,$(OBJCOUNT)),$(OBJCOUNT)))

# Builds phc's
pch:
	@printf "[pch] compiling $(PHC)..."
	@$(CC) $(PHCFLAGS)
	@printf "\b\b done!\n"

stdlib: makedirs
	@$(MAKE) --no-print-directory -f $(STDLIB_SRC_DIR)/Makefile stdlib

.PHONY: clean-stdlib
clean-stdlib:
	@$(MAKE) --no-print-directory -f $(STDLIB_SRC_DIR)/Makefile clean

################### Cleaning rules for Unix-based OS ###################

# Cleans complete project
.PHONY: clean
clean:
	@# $(RM) $(DELOBJ) $(DEP) $(APP)
	@$(RM) -rf $(OBJDIR)
	@$(RM) -rf $(BINDIR)

.PHONY: test
test: $(APP)
	@printf "============= Running \"$(APP)\" =============\n\n"
	@$(APP) test/test.evi -o bin/test $(args) && \
	\
	printf "============= Running \"bin/test\" ===========\n\n" && \
	bin/test && \
	\
	printf "\n\n============ Exited with code $$? ============\n" && \
	rm bin/test \
	|| echo "============ Test failed with code $$? ============"

	@rm test/test.evi.*.o 2>/dev/null && echo "Object file left over! (now cleaned)" || true

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

.PHONY: debug-no-fold
debug-no-fold: CXXFLAGS += -D DEBUG_NO_FOLD
debug-no-fold: debug

.PHONY: valgrind
valgrind: debug $(APP)
	@valgrind bin/evi test/test.evi -o bin/test $(args)

git:
	git add --all
# 	git status
	git commit -m $$(test "$(msg)" && echo '$(msg)' || echo upload)
	git push origin main

newfile:
	@test $(name) || ( echo "basename not given! ('make newfile name=BASENAME')"; false )
	touch $(SRCDIR)/$(name).cpp
	touch $(HEADERDIR)/$(name).hpp

man:
	@cp tools/evi-man tools/evi-man.tmp
	@DATE=$$(date +"%a %d, %Y") && sed -i -e "s/<<<DATE>>>/$$DATE/g" tools/evi-man.tmp
	@gzip tools/evi-man.tmp
	@sudo mv tools/evi-man.tmp.gz /usr/share/man/man1/evi.1.gz
