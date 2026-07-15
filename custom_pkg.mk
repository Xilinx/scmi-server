#
# Copyright (c) 2026, Advanced Micro Devices Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#

# Get the current package path
SCMI_SERVER_ROOT := $(dir $(lastword $(MAKEFILE_LIST)))
SCMI_SERVER_ROOT := $(patsubst %/,%,$(SCMI_SERVER_ROOT))

# Define SCMI_SERVER_PATH for internal package usage
SCMI_SERVER_PATH := $(SCMI_SERVER_ROOT)
$(eval $(call add_define,SCMI_SERVER_PATH))

# =============================================================================
# Version Information - Git Commit and Build Timestamp
# =============================================================================

# Get git commit information from scmi-server repository
SCMI_GIT_COMMIT := $(shell cd $(SCMI_SERVER_ROOT) && git rev-parse --short HEAD 2>/dev/null || echo "unknown")
SCMI_GIT_COMMIT_LONG := $(shell cd $(SCMI_SERVER_ROOT) && git rev-parse HEAD 2>/dev/null || echo "unknown")
SCMI_GIT_BRANCH := $(shell cd $(SCMI_SERVER_ROOT) && git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")
SCMI_GIT_DIRTY := $(shell cd $(SCMI_SERVER_ROOT) && git diff --quiet 2>/dev/null || echo "-dirty")
SCMI_GIT_DATE := $(shell cd $(SCMI_SERVER_ROOT) && git log -1 --format=%cd --date=short 2>/dev/null || echo "unknown")

# Get build timestamp (when make is executed)
SCMI_BUILD_DATE := $(shell date +'%Y-%m-%d')
SCMI_BUILD_TIME := $(shell date +'%H:%M:%S')
SCMI_BUILD_TIMESTAMP := $(SCMI_BUILD_DATE) $(SCMI_BUILD_TIME)

# Combined version string
SCMI_PKG_VERSION := $(SCMI_GIT_COMMIT)$(SCMI_GIT_DIRTY)

# Export as preprocessor defines (quoted strings)
$(eval $(call add_define_val,SCMI_GIT_COMMIT,"\"$(SCMI_GIT_COMMIT)\""))
$(eval $(call add_define_val,SCMI_GIT_COMMIT_LONG,"\"$(SCMI_GIT_COMMIT_LONG)\""))
$(eval $(call add_define_val,SCMI_GIT_BRANCH,"\"$(SCMI_GIT_BRANCH)\""))
$(eval $(call add_define_val,SCMI_GIT_DATE,"\"$(SCMI_GIT_DATE)\""))
$(eval $(call add_define_val,SCMI_PKG_VERSION,"\"$(SCMI_PKG_VERSION)\""))
$(eval $(call add_define_val,SCMI_BUILD_DATE,"\"$(SCMI_BUILD_DATE)\""))
$(eval $(call add_define_val,SCMI_BUILD_TIME,"\"$(SCMI_BUILD_TIME)\""))
$(eval $(call add_define_val,SCMI_BUILD_TIMESTAMP,"\"$(SCMI_BUILD_TIMESTAMP)\""))

# Print version information during build
$(info ================================================================================)
$(info SCMI Server Package Information:)
$(info   Branch:        $(SCMI_GIT_BRANCH))
$(info   Last Commit:   $(SCMI_GIT_DATE))
$(info   Version:       $(SCMI_PKG_VERSION))
$(info   Build Date:    $(SCMI_BUILD_DATE))
$(info   Build Time:    $(SCMI_BUILD_TIME))
$(info   Package Path:  $(SCMI_SERVER_ROOT))
$(info ================================================================================)

# =============================================================================
# Configuration Flags
# =============================================================================

# =============================================================================
# Feature Configuration - Conditionally enable SCMI protocols
# =============================================================================


# =============================================================================
# Include Paths
# =============================================================================

# 1. Package header directory (if exists)
ifneq ($(wildcard $(SCMI_SERVER_ROOT)/inc),)
    INCLUDES += -I$(SCMI_SERVER_ROOT)/inc
endif

# 2. Package source directory
INCLUDES += -I$(SCMI_SERVER_ROOT)/src

# 3. TF-A Versal2 platform includes
INCLUDES += \
    -Iplat/amd/versal2/include \
    -Iplat/amd/common/include \
    -Iplat/amd/common

# 4. TF-A common includes
INCLUDES += \
    -Iinclude \
    -Iinclude/common \
    -Iinclude/drivers \
    -Iinclude/lib \
    -Iinclude/lib/libc \
    -Iinclude/plat/common

# =============================================================================
# Library Paths
# =============================================================================

ifneq ($(wildcard $(SCMI_SERVER_ROOT)/lib),)
    LDFLAGS += -L$(SCMI_SERVER_ROOT)/lib
    LDLIBS += $(addprefix -l,$(patsubst lib%.a,%,$(notdir $(wildcard $(SCMI_SERVER_ROOT)/lib/*.a))))
endif

# =============================================================================
# Source Files - Organized by layer
# =============================================================================

# SCMI protocol implementations
SCMI_PROTOCOL_SRCS := \
    $(SCMI_SERVER_ROOT)/src/protocols/base.c \
    $(SCMI_SERVER_ROOT)/src/protocols/clock.c \
    $(SCMI_SERVER_ROOT)/src/protocols/power_domain.c \
    $(SCMI_SERVER_ROOT)/src/protocols/reset_domain.c
#   $(SCMI_SERVER_ROOT)/src/protocols/sensor.c  # Sensor not implemented for Versal2

# SCMI transport layer
SCMI_TRANSPORT_SRCS := \
    $(SCMI_SERVER_ROOT)/src/transport/entry.c \
    $(SCMI_SERVER_ROOT)/src/transport/smt.c

# Platform-specific implementation
SCMI_PLATFORM_SRCS := \
    $(SCMI_SERVER_ROOT)/src/platform/scmi.c \
    $(SCMI_SERVER_ROOT)/src/platform/scmi_clock.c \
    $(SCMI_SERVER_ROOT)/src/platform/scmi_clock_map.c \
    $(SCMI_SERVER_ROOT)/src/platform/scmi_reset_domain.c \
    $(SCMI_SERVER_ROOT)/src/platform/scmi_power_domain.c \
    $(SCMI_SERVER_ROOT)/src/platform/custom_svc.c \
    $(SCMI_SERVER_ROOT)/src/platform/pm_api.c

# Combine all SCMI sources
SCMI_SERVER_SRCS := $(SCMI_PROTOCOL_SRCS) $(SCMI_TRANSPORT_SRCS) $(SCMI_PLATFORM_SRCS)

# Add to BL31 build
BL31_SOURCES += $(SCMI_SERVER_SRCS)
