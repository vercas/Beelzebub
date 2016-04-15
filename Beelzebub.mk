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

	PRECOMPILER_FLAGS	+= __BEELZEBUB__ARCH_IA32PAE __BEELZEBUB__ARCH_IA32 
	PRECOMPILER_FLAGS	+= __BEELZEBUB__ARCH_X86 

##############
# 32-bit x86 #
else ifneq (,$(findstring ia32,$(MAKECMDGOALS)))
	ARC					:= ia32
	AUX					:= x86

	PRECOMPILER_FLAGS	+= __BEELZEBUB__ARCH_IA32 __BEELZEBUB__ARCH_X86 

endif

################
#	SETTINGS   #
################

####################################### CONFIGURATION ##########

#########
# Debug #
ifneq (,$(findstring conf-profile,$(MAKECMDGOALS)))
	PRECOMPILER_FLAGS	+= __BEELZEBUB__PROFILE __BEELZEBUB__RELEASE 
	PRECOMPILER_FLAGS	+= __JEGUDIEL__PROFILE __JEGUDIEL__RELEASE 

	SETTINGS			+= conf-profile
else ifneq (,$(findstring conf-release,$(MAKECMDGOALS)))
	PRECOMPILER_FLAGS	+= __BEELZEBUB__RELEASE 
	PRECOMPILER_FLAGS	+= __JEGUDIEL__RELEASE 

	SETTINGS			+= conf-release
else
	#	Yes, debug is default!

	PRECOMPILER_FLAGS	+= __BEELZEBUB__DEBUG 
	PRECOMPILER_FLAGS	+= __JEGUDIEL__DEBUG 

	SETTINGS			+= conf-debug
endif

####################################### FLAGS ##########

###############
# SMP disable #
ifneq (,$(findstring no-smp,$(MAKECMDGOALS)))
	PRECOMPILER_FLAGS	+= __BEELZEBUB_SETTINGS_NO_SMP 
	PRECOMPILER_FLAGS	+= __JEGUDIEL_SETTINGS_NO_SMP 

	SETTINGS			+= no-smp
else
	PRECOMPILER_FLAGS	+= __BEELZEBUB_SETTINGS_SMP 
	PRECOMPILER_FLAGS	+= __JEGUDIEL_SETTINGS_SMP 

	SETTINGS			+= smp
endif

##########################
# Don't inline spinlocks #
ifneq (,$(findstring no-inline-spinlocks,$(MAKECMDGOALS)))
	PRECOMPILER_FLAGS	+= __BEELZEBUB_SETTINGS_NO_INLINE_SPINLOCKS 

	SETTINGS			+= no-inline-spinlocks
else
	PRECOMPILER_FLAGS	+= __BEELZEBUB_SETTINGS_INLINE_SPINLOCKS 

	SETTINGS			+= inline-spinlocks
endif

#################
# No Unit Tests #
ifneq (,$(findstring no-unit-tests,$(MAKECMDGOALS)))
	PRECOMPILER_FLAGS	+= __BEELZEBUB_SETTINGS_NO_UNIT_TESTS 

	SETTINGS			+= no-unit-tests
else ifneq (,$(findstring unit-tests-quiet,$(MAKECMDGOALS)))
	PRECOMPILER_FLAGS	+= __BEELZEBUB_SETTINGS_UNIT_TESTS 
	PRECOMPILER_FLAGS	+= __BEELZEBUB_SETTINGS_UNIT_TESTS_QUIET 

	SETTINGS			+= unit-tests-quiet
else
	PRECOMPILER_FLAGS	+= __BEELZEBUB_SETTINGS_UNIT_TESTS 

	SETTINGS			+= unit-tests
endif

####################################### Tests ##########

############
# ALL!!!1! #
ifneq (,$(findstring test-all,$(MAKECMDGOALS)))
	PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_ALL 

	#	Aye, these are added explicitly.
	PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_MT 
	PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_STR 
	PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_OBJA 
	PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_METAP 
	PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_EXCP 
	PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_APP 
	PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_STACKINT 
	PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_AVL_TREE 
	PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_TERMINAL 
	PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_CMDO 
	PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_FPU 
	PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_BIGINT 

	SETTINGS			+= test-all
else
	ifneq (,$(findstring test-mt,$(MAKECMDGOALS)))
		PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_MT 

		SETTINGS			+= test-mt
	endif

	ifneq (,$(findstring test-str,$(MAKECMDGOALS)))
		PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_STR 

		SETTINGS			+= test-str
	endif

	ifneq (,$(findstring test-obja,$(MAKECMDGOALS)))
		PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_OBJA 

		SETTINGS			+= test-obja
	endif

	ifneq (,$(findstring test-metap,$(MAKECMDGOALS)))
		PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_METAP 

		SETTINGS			+= test-metap
	endif

	ifneq (,$(findstring test-excp,$(MAKECMDGOALS)))
		PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_EXCP 

		SETTINGS			+= test-excp
	endif

	ifneq (,$(findstring test-app,$(MAKECMDGOALS)))
		PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_APP __BEELZEBUB__TEST_MT 

		SETTINGS			+= test-app test-mt 
	endif

	ifneq (,$(findstring test-stackint,$(MAKECMDGOALS)))
		PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_STACKINT 

		SETTINGS			+= test-stackint 
	endif

	ifneq (,$(findstring test-avl-tree,$(MAKECMDGOALS)))
		PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_AVL_TREE 

		SETTINGS			+= test-avl-tree 
	endif

	ifneq (,$(findstring test-terminal,$(MAKECMDGOALS)))
		PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_TERMINAL 

		SETTINGS			+= test-terminal 
	endif

	ifneq (,$(findstring test-cmdo,$(MAKECMDGOALS)))
		PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_CMDO 

		SETTINGS			+= test-cmdo 
	endif

	ifneq (,$(findstring test-fpu,$(MAKECMDGOALS)))
		PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_FPU 

		SETTINGS			+= test-fpu 
	endif

	ifneq (,$(findstring test-bigint,$(MAKECMDGOALS)))
		PRECOMPILER_FLAGS	+= __BEELZEBUB__TEST_BIGINT 

		SETTINGS			+= test-bigint 
	endif
endif

####################################### PRECOMPILER FLAGS ##########

GCC_PRECOMPILER_FLAGS	:= $(patsubst %,-D %,$(PRECOMPILER_FLAGS))

# When architecture files are present...
ifneq (,$(ARC))
	GCC_PRECOMPILER_FLAGS	+= -D __BEELZEBUB__ARCH=$(ARC)
endif

# When auxiliary files are present...
ifneq (,$(AUX))
	GCC_PRECOMPILER_FLAGS	+= -D __BEELZEBUB__AUX=$(AUX)
endif

####################################### COMMON PATHS ##########

# Binary blobs
KERNEL_BIN				:= $(KERNEL_NAME).$(ARC).bin

# Sysroot
SYSROOT					:= $(PREFIX)/sysroot.$(ARC)

# Installation
KERNEL_INSTALL_DIR		:= $(PREFIX)/bin
KERNEL_INSTALL_PATH		:= $(KERNEL_INSTALL_DIR)/$(KERNEL_BIN)

# ISO
ISO_PATH				:= $(PREFIX)/$(KERNEL_NAME).$(ARC).$(AUX).iso

####################################### WRAP UP ##########

$(SETTINGS):: $(ARC)
	@ true
#	Just makin' sure these don't error out. Adding '@ true' stops GNU Make from
#	displaying 'Nothing to be done for ...', which gets annoying when there are
#	more settings.
