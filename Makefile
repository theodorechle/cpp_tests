CPP_C:=g++
CPP_FLAGS:=-std=c++23 -Wall -Werror -g -MMD -MP -Iinclude -Isrc -fpic
LIB_FLAGS:=-shared
BIN_DIR:=bin
OBJ_DIR:=obj
SRC_DIR:=src

INSTALL_PATH?=/usr/local
LIB_INSTALL_PATH:=$(INSTALL_PATH)/lib
INCLUDE_INSTALL_PATH:=$(INSTALL_PATH)/include

NAME:=cpptests
MAJOR_VERSION:=1
VERSION:=$(MAJOR_VERSION).0.0

LIB:=lib$(NAME).so.$(VERSION)
MAJOR_LIB:=lib$(NAME).so.$(MAJOR_VERSION)
LIB_NO_VERSION:=lib$(NAME).so

SRC:=$(wildcard $(SRC_DIR)/*.cpp)
OBJ:=$(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC))
DEPS:=$(OBJ:.o=.d)

.PHONY: lib clean install uninstall

ifeq ($(DEBUG),1)
CPP_FLAGS += -DDEBUG
endif

lib: $(BIN_DIR)/$(LIB)

$(BIN_DIR)/$(LIB): $(OBJ)
	@mkdir -p $(@D)
	$(CPP_C) $(LIB_FLAGS) -Wl,-soname,$(MAJOR_LIB) -o $@ $^

	ln -sfn $(LIB) $(BIN_DIR)/$(MAJOR_LIB)
	ln -sfn $(MAJOR_LIB) $(BIN_DIR)/$(LIB_NO_VERSION)

# Rule for compiling all object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CPP_C) $(CPP_FLAGS) -c $< -o $@

-include $(DEPS)

# Clean all generated files
clean:
	rm -rf bin obj

install: $(BIN_DIR)/$(LIB)
	install -d $(LIB_INSTALL_PATH)

	# remove old minor version if existing
	rm -f $(LIB_INSTALL_PATH)/$(MAJOR_LIB)*

	install -m 755 $(BIN_DIR)/$(LIB) $(LIB_INSTALL_PATH)/$(LIB)
	install -d $(INCLUDE_INSTALL_PATH)/$(NAME)

	ln -sfn $(LIB) $(LIB_INSTALL_PATH)/$(MAJOR_LIB)
	ln -sfn $(MAJOR_LIB) $(LIB_INSTALL_PATH)/$(LIB_NO_VERSION)

	install -m 644 include/$(NAME)/*.hpp $(INCLUDE_INSTALL_PATH)/$(NAME)/

uninstall:
	rm $(LIB_INSTALL_PATH)/$(LIB) $(LIB_INSTALL_PATH)/$(MAJOR_LIB) $(LIB_INSTALL_PATH)/$(LIB_NO_VERSION)
	rm -r $(INCLUDE_INSTALL_PATH)/$(NAME)
