CXX := g++

SRC_DIR := ./src
OBJ_DIR := ./obj
OUT_DIR := ./bin
DEP_DIR := ./.dep
OUT_NAME := bfbrute
# CXXFLAGS := -std=c++2a -g -O0
# CXXFLAGS := -std=c++2a -g -O2 -DNDEBUG
CXXFLAGS := -std=c++2a -O2 -DNDEBUG
LDFLAGS := -pthread
INC_DIRS := ./src
LNK_DIRS := 
LNK_IGNORE := 

OUT_FILE := $(OUT_DIR)/$(OUT_NAME)

SRC_FILES := $(shell find $(SRC_DIR) -name '*.cpp')
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))
OBJ_FILES += $(foreach DIR,$(LNK_DIRS),$(shell find $(DIR) -name '*.o'))
OBJ_FILES := $(filter-out $(shell find $(OBJ_FILES) -name '$(LNK_IGNORE)'),$(OBJ_FILES))

EMPTY :=
SPACE := $(EMPTY) $(EMPTY)
INCLUDE_FLAG := $(EMPTY) -I$(EMPTY)
CXXFLAGS += $(INCLUDE_FLAG)$(subst $(SPACE),$(INCLUDE_FLAG),$(INC_DIRS))

ifeq (,$(wildcard $(DEP_DIR)))
$(shell mkdir -p $(DEP_DIR))
endif

ifeq (,$(wildcard $(OBJ_DIR)))
$(shell mkdir -p $(OBJ_DIR))
endif

ifeq (,$(wildcard $(OUT_DIR)))
$(shell mkdir -p $(OUT_DIR))
endif

# always rebuild for templated headers

$(DEP_DIR)/%.d: $(SRC_DIR)/%.cpp .FORCE
ifeq (,$(wildcard $(dir $@)))
	mkdir -p $(dir $@) 
endif
	$(CXX) $(CXXFLAGS) -MM -MF $@ -MP $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(DEP_DIR)/%.d
ifeq (,$(wildcard $(dir $@)))
	mkdir -p "$(dir $@)"
endif
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUT_FILE): $(OBJ_FILES) .FORCE
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJ_FILES)

.FORCE:

.PHONY: .FORCE