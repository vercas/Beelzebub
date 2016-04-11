####################################### DEPENDENCIES ##########

#################################
# Obtain depdendency file names #
DEPENDENCIES = $(OBJECTS:%.o=%.d)

####################################### NORMAL FILES ##########

#####################
# Build a NASM file #
$(BUILD_DIR)/%.asm.o: $(SRC_DIR)/%.asm
	@ echo "/STD/Asm:" $< ">" $@
	@ mkdir -p $(@D)
	@ $(PRE_COMPILATION)    $(AS) $(ASFLAGS) $< -o $@    $(POST_COMPILATION)

####################
# Build a GAS file #
$(BUILD_DIR)/%.s.o: $(SRC_DIR)/%.s
	@ echo "/STD/Gas:" $< ">" $@
	@ mkdir -p $(@D)
	@ $(PRE_COMPILATION)    $(GAS) $(GASFLAGS) -c $< -o $@    $(POST_COMPILATION)

##################
# Build a C file #
$(BUILD_DIR)/%.c.o: $(SRC_DIR)/%.c
	@ echo "/STD/C__:" $< ">" $@
	@ mkdir -p $(@D)
	@ $(PRE_COMPILATION)    $(CC) $(CFLAGS) -MMD -MP -c $< -o $@    $(POST_COMPILATION)
# @ $(PRE_COMPILATION)    $(CC) $(CFLAGS) -Wa,-aslh -c $< -o $@    $(POST_COMPILATION) 1> $@.s

####################
# Build a C++ file #
$(BUILD_DIR)/%.cpp.o: $(SRC_DIR)/%.cpp
	@ echo "/STD/C++:" $< ">" $@
	@ mkdir -p $(@D)
	@ $(PRE_COMPILATION)    $(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@    $(POST_COMPILATION)
# @ $(PRE_COMPILATION)    $(CXX) $(CXXFLAGS) -Wa,-aslh -c $< -o $@    $(POST_COMPILATION) 1> $@.s

####################################### ARCHITECTURE FILES ##########

#####################
# Build a NASM file #
$(BUILD_DIR)/%.asm.arc.o: $(ARC_DIR)/$(ARC)/src/%.asm
	@ echo "/ARC/Asm:" $< ">" $@
	@ mkdir -p $(@D)
	@ $(PRE_COMPILATION)    $(AS) $(ASFLAGS) $< -o $@    $(POST_COMPILATION)

####################
# Build a GAS file #
$(BUILD_DIR)/%.s.arc.o: $(ARC_DIR)/$(ARC)/src/%.s
	@ echo "/ARC/Gas:" $< ">" $@
	@ mkdir -p $(@D)
	@ $(PRE_COMPILATION)    $(GAS) $(GASFLAGS) -c $< -o $@    $(POST_COMPILATION)

##################
# Build a C file #
$(BUILD_DIR)/%.c.arc.o: $(ARC_DIR)/$(ARC)/src/%.c
	@ echo "/ARC/C__:" $< ">" $@
	@ mkdir -p $(@D)
	@ $(PRE_COMPILATION)    $(CC) $(CFLAGS) -MMD -MP -c $< -o $@    $(POST_COMPILATION)
# @ $(PRE_COMPILATION)    $(CC) $(CFLAGS) -Wa,-aslh -c $< -o $@    $(POST_COMPILATION) 1> $@.s

####################
# Build a C++ file #
$(BUILD_DIR)/%.cpp.arc.o: $(ARC_DIR)/$(ARC)/src/%.cpp
	@ echo "/ARC/C++:" $< ">" $@
	@ mkdir -p $(@D)
	@ $(PRE_COMPILATION)    $(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@    $(POST_COMPILATION)
# @ $(PRE_COMPILATION)    $(CXX) $(CXXFLAGS) -Wa,-aslh -c $< -o $@    $(POST_COMPILATION) 1> $@.s

####################################### AUXILIARY FILES ##########

#####################
# Build a NASM file #
$(BUILD_DIR)/%.asm.aux.o: $(AUX_DIR)/$(AUX)/src/%.asm
	@ echo "/AUX/Asm:" $< ">" $@
	@ mkdir -p $(@D)
	@ $(PRE_COMPILATION)    $(AS) $(ASFLAGS) $< -o $@    $(POST_COMPILATION)

####################
# Build a GAS file #
$(BUILD_DIR)/%.s.aux.o: $(AUX_DIR)/$(AUX)/src/%.s
	@ echo "/AUX/Gas:" $< ">" $@
	@ mkdir -p $(@D)
	@ $(PRE_COMPILATION)    $(GAS) $(GASFLAGS) -c $< -o $@    $(POST_COMPILATION)

##################
# Build a C file #
$(BUILD_DIR)/%.c.aux.o: $(AUX_DIR)/$(AUX)/src/%.c
	@ echo "/AUX/C__:" $< ">" $@
	@ mkdir -p $(@D)
	@ $(PRE_COMPILATION)    $(CC) $(CFLAGS) -MMD -MP -c $< -o $@    $(POST_COMPILATION)
# @ $(PRE_COMPILATION)    $(CC) $(CFLAGS) -Wa,-aslh -c $< -o $@    $(POST_COMPILATION) 1> $@.s

####################
# Build a C++ file #
$(BUILD_DIR)/%.cpp.aux.o: $(AUX_DIR)/$(AUX)/src/%.cpp
	@ echo "/AUX/C++:" $< ">" $@
	@ mkdir -p $(@D)
	@ $(PRE_COMPILATION)    $(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@    $(POST_COMPILATION)
# @ $(PRE_COMPILATION)    $(CXX) $(CXXFLAGS) -Wa,-aslh -c $< -o $@    $(POST_COMPILATION) 1> $@.s

####################################### DEPENDENCIES ##########

################################
# Include all dependency files #
-include $(DEPENDENCIES)
