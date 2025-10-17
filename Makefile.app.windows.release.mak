DIR := $(subst /,\,${CURDIR})
BUILD_DIR := bin/_RELEASE
VENDOR_DIR := vendor
OBJ_DIR := obj

TITLE := Incendium
ASSEMBLY := app
EXTENSION := .exe
COMPILER_FLAGS := -std=c++23 -Werror=vla -Wall -Wextra -Wpedantic -Wno-unused-function -O3
INCLUDE_FLAGS := -Ivendor/include -Iapp/src
LINKER_FLAGS := -static -mwindows                                                                   \
                -L$(OBJ_DIR)/ -L$(VENDOR_DIR)/lib/ -L$(BUILD_DIR) -L$(VENDOR_DIR)/lib/steam/win64/  \
                -lsdkencryptedappticket64 -lsteam_api64 -lraylib -lucrtbase -lGdi32 -lWinMM -lUser32 -lShell32 -static-libstdc++ -libcrypto -libssl
DEFINES := -D_RELEASE

# Make does not offer a recursive wildcard function, so here's one:
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

SRC_FILES := $(call rwildcard,$(ASSEMBLY)/,*.cpp) # Get all .cpp files
DIRECTORIES := \$(ASSEMBLY)\src $(subst $(DIR),,$(shell dir $(ASSEMBLY)\src /S /AD /B | findstr /i src)) # Get all directories under src.
OBJ_FILES := $(SRC_FILES:%=$(OBJ_DIR)/%.o) # Get all compiled .cpp.o objects

all: scaffold compile link

.PHONY: scaffold
scaffold: # create build directory
	@echo Scaffolding folder structure...
	-@setlocal enableextensions enabledelayedexpansion && mkdir $(addprefix $(OBJ_DIR), $(DIRECTORIES)) 2>NUL || cd .
	@echo Done.

.PHONY: link
link: scaffold $(OBJ_FILES) # link
	@echo Linking $(ASSEMBLY)...
	@clang++ $(OBJ_FILES) -o $(BUILD_DIR)/$(TITLE)$(EXTENSION) $(LINKER_FLAGS) $(BUILD_DIR)/icon.res

.PHONY: compile
compile: #compile .cpp files
	@echo Compiling...

.PHONY: clean
clean: # clean build directory
	if exist $(BUILD_DIR)\$(TITLE)$(EXTENSION) del $(BUILD_DIR)\$(TITLE)$(EXTENSION)
	rmdir /s /q $(OBJ_DIR)\$(ASSEMBLY)

$(OBJ_DIR)/%.cpp.o: %.cpp # compile .cpp to .cpp.o object
	@echo   $<...
	@clang++ $< $(COMPILER_FLAGS) -c -o $@ $(DEFINES) $(INCLUDE_FLAGS)
	