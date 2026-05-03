# Project Name
TARGET = NzStretch

# Library Locations
NOIS_DIR         = ../../../
LIBDAISY_DIR     = $(NOIS_DIR)/examples/libs/libdaisy
UMM_DIR          = $(NOIS_DIR)/examples/libs/umm_malloc
CMSIS_DIR        = $(LIBDAISY_DIR)/Drivers/CMSIS
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core

# App type
# APP_TYPE = BOOT_QSPI

# Standard
CPP_STANDARD = -std=gnu++20

# Sources
CPP_SOURCES = \
	$(NOIS_DIR)/examples/nois-stretch/versio/src/VersioStretch.cpp \
	$(NOIS_DIR)/src/effect/NoisTimeStretcher.cpp
C_SOURCES += \
	$(UMM_DIR)/src/umm_malloc.c

# Includes
C_INCLUDES += \
	-I $(NOIS_DIR)/libs/debugbreak \
	-I $(NOIS_DIR)/include \
	-I $(NOIS_DIR)/src \
	-I $(UMM_DIR)/src \
	-I $(NOIS_DIR)/examples/common/versio/include

include $(SYSTEM_FILES_DIR)/Makefile