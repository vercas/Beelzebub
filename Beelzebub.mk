###
# I assume $(PREFIX) and $(MAKECMDGOALS) are set!
# Also, $(KERNEL_NAME)!
###

##########
# Target #
ARC			:= ''
AUX			:= ''

######################################
#	ARCHITECTURE-SPECIFIC SETTINGS   #
######################################

##############
# 64-bit x86 #
ifneq (,$(findstring amd64,$(MAKECMDGOALS)))
	ARC			:= amd64
	AUX			:= x86

####################################
# 32-bit x86 with 36-bit addresses #
else ifneq (,$(findstring ia32pae,$(MAKECMDGOALS)))
	ARC			:= ia32pae
	AUX			:= x86

##############
# 32-bit x86 #
else ifneq (,$(findstring ia32,$(MAKECMDGOALS)))
	ARC			:= ia32
	AUX			:= x86

endif

####################################### COMMON PATHS ##########

# Binary blobs
KERNEL_BIN				:= $(KERNEL_NAME).$(ARC).bin

# Installation
KERNEL_INSTALL_DIR		:= $(PREFIX)/bin
KERNEL_INSTALL_PATH		:= $(KERNEL_INSTALL_DIR)/$(KERNEL_BIN)

# ISO
ISO_PATH				:= $(PREFIX)/$(KERNEL_NAME).$(ARC).$(AUX).iso
