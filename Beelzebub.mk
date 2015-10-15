###
# I assume $(PREFIX) and $(MAKECMDGOALS) are set!
###

##########
# Target #
KERNEL_NAME				:= beelzebub
ARC						:=
AUX						:=

############
# Settings #
PRECOMPILER_FLAGS		:= __BEELZEBUB 
SETTINGS 				:=

SMP						:= true
#	Defaults to enabled.

################################################
#	ARCHITECTURE-SPECIFIC SETTINGS AND FLAGS   #
################################################

##############
# 64-bit x86 #
ifneq (,$(findstring amd64,$(MAKECMDGOALS)))
	ARC					:= amd64
	AUX					:= x86

	PRECOMPILER_FLAGS	+= __BEELZEBUB__ARCH_AMD64 __BEELZEBUB__ARCH_X86 

####################################
# 32-bit x86 with 36-bit addresses #
else ifneq (,$(findstring ia32pae,$(MAKECMDGOALS)))
	ARC					:= ia32pae
	AUX					:= x86

	PRECOMPILER_FLAGS	+= __BEELZEBUB__ARCH_IA32PAE __BEELZEBUB__ARCH_IA32 __BEELZEBUB__ARCH_X86 

##############
# 32-bit x86 #
else ifneq (,$(findstring ia32,$(MAKECMDGOALS)))
	ARC					:= ia32
	AUX					:= x86

	PRECOMPILER_FLAGS	+= __BEELZEBUB__ARCH_IA32 __BEELZEBUB__ARCH_X86 

endif

####################################### SETTINGS ##########

###############
# SMP disable #
ifneq (,$(findstring no-smp,$(MAKECMDGOALS)))
	SMP					:=

	PRECOMPILER_FLAGS	+= __BEELZEBUB_SETTINGS_NO_SMP 

	SETTINGS			+= no-smp
else
	PRECOMPILER_FLAGS	+= __BEELZEBUB_SETTINGS_SMP 

	SETTINGS			+= smp
endif

####################################### PRECOMPILER FLAGS ##########

GCC_PRECOMPILER_FLAGS	:= $(patsubst %,-D %,$(PRECOMPILER_FLAGS))

# When architecture files are present...
ifneq ($(ARC),'')
	GCC_PRECOMPILER_FLAGS	+= -D __BEELZEBUB__ARCH=$(ARC)
endif

# When auxiliary files are present...
ifneq ($(AUX),'')
	GCC_PRECOMPILER_FLAGS	+= -D __BEELZEBUB__AUX=$(AUX)
endif

####################################### COMMON PATHS ##########

# Binary blobs
KERNEL_BIN				:= $(KERNEL_NAME).$(ARC).bin

# Installation
KERNEL_INSTALL_DIR		:= $(PREFIX)/bin
KERNEL_INSTALL_PATH		:= $(KERNEL_INSTALL_DIR)/$(KERNEL_BIN)

# ISO
ISO_PATH				:= $(PREFIX)/$(KERNEL_NAME).$(ARC).$(AUX).iso
