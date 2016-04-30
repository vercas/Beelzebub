# Standard
INCFLAGS	:= -I$(INC_DIR) 

OBJECTS		:= $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.c.o,$(shell find $(SRC_DIR) -name "*.c"))
OBJECTS		+= $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.cpp.o,$(shell find $(SRC_DIR) -name "*.cpp"))
OBJECTS		+= $(patsubst $(SRC_DIR)/%.asm,$(BUILD_DIR)/%.asm.o,$(shell find $(SRC_DIR) -name "*.asm"))
OBJECTS		+= $(patsubst $(SRC_DIR)/%.s,$(BUILD_DIR)/%.s.o,$(shell find $(SRC_DIR) -name "*.s"))

# When architecture-specific files are present...
ifneq (,$(ARC))
	INCFLAGS	+= -I$(ARC_DIR)/$(ARC)/inc 

	OBJECTS		+= $(patsubst $(ARC_DIR)/$(ARC)/src/%.c,$(BUILD_DIR)/%.c.arc.o,$(shell find $(ARC_DIR)/$(ARC)/src -name "*.c"))
	OBJECTS		+= $(patsubst $(ARC_DIR)/$(ARC)/src/%.cpp,$(BUILD_DIR)/%.cpp.arc.o,$(shell find $(ARC_DIR)/$(ARC)/src -name "*.cpp"))
	OBJECTS		+= $(patsubst $(ARC_DIR)/$(ARC)/src/%.asm,$(BUILD_DIR)/%.asm.arc.o,$(shell find $(ARC_DIR)/$(ARC)/src -name "*.asm"))
	OBJECTS		+= $(patsubst $(ARC_DIR)/$(ARC)/src/%.s,$(BUILD_DIR)/%.s.arc.o,$(shell find $(ARC_DIR)/$(ARC)/src -name "*.s"))
endif

# When auxiliary files are present...
ifneq (,$(AUX))
	INCFLAGS	+= -I$(AUX_DIR)/$(AUX)/inc 

	OBJECTS		+= $(patsubst $(AUX_DIR)/$(AUX)/src/%.c,$(BUILD_DIR)/%.c.aux.o,$(shell find $(AUX_DIR)/$(AUX)/src -name "*.c"))
	OBJECTS		+= $(patsubst $(AUX_DIR)/$(AUX)/src/%.cpp,$(BUILD_DIR)/%.cpp.aux.o,$(shell find $(AUX_DIR)/$(AUX)/src -name "*.cpp"))
	OBJECTS		+= $(patsubst $(AUX_DIR)/$(AUX)/src/%.asm,$(BUILD_DIR)/%.asm.aux.o,$(shell find $(AUX_DIR)/$(AUX)/src -name "*.asm"))
	OBJECTS		+= $(patsubst $(AUX_DIR)/$(AUX)/src/%.s,$(BUILD_DIR)/%.s.aux.o,$(shell find $(AUX_DIR)/$(AUX)/src -name "*.s"))
endif

# INCFLAGS	+= -I$(INC_COMMON)/common 
