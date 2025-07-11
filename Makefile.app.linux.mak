
BUILD_DIR := bin
VENDOR_DIR := vendor
OBJ_DIR := obj

TITLE := Incendium
ASSEMBLY := app
EXTENSION := 
COMPILER_FLAGS := -g -MD -Werror=vla -Wall -Wextra -Wpedantic -std=c23
INCLUDE_FLAGS := -I./app/src -I./vendor/include
LINKER_FLAGS := -L./$(OBJ_DIR)/ -L./$(VENDOR_DIR)/lib/ -L./$(BUILD_DIR) -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
DEFINES := -D_DEBUG
# Make does not offer a recursive wildcard function, so here's one:
#rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

SRC_FILES := $(shell find $(ASSEMBLY) -name *.c)		# .c files
DIRECTORIES := $(shell find $(ASSEMBLY) -type d)		# directories with .h files
OBJ_FILES := $(SRC_FILES:%=$(OBJ_DIR)/%.o)		# compiled .o objects

all: scaffold compile link

.PHONY: scaffold
scaffold: # create build directory
	@echo Scaffolding folder structure...
	@mkdir -p $(addprefix $(OBJ_DIR)/,$(DIRECTORIES))
	@echo Done.

.PHONY: link
link: scaffold $(OBJ_FILES) # link
	@echo Linking $(ASSEMBLY)...
	@gcc $(OBJ_FILES) -o $(BUILD_DIR)/$(TITLE)$(EXTENSION) $(LINKER_FLAGS)

.PHONY: compile
compile: #compile .c files
	@echo Compiling...
	
.PHONY: clean
clean: # clean build directory
	rm -rf $(BUILD_DIR)\$(ASSEMBLY)
	rm -rf $(OBJ_DIR)\$(ASSEMBLY)

$(OBJ_DIR)/%.c.o: %.c # compile .c to .o object
	@echo   $<...
	@gcc $< $(COMPILER_FLAGS) -c -o $@ $(DEFINES) $(INCLUDE_FLAGS)

-include $(OBJ_FILES:.o=.d) e
