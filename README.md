# scmi-server

This repository contains a SCMI (System Control and Management Interface) server implementation for AMD Versal2 platforms, integrated with Trusted Firmware-A (TF-A) via the Custom Package framework.

## Overview

The scmi-server package provides:
- **SCMI protocol support** for Base, Clock, Power Domain (v2), and Reset Domain (v1) protocols
- **SMT (Shared Memory Transport)** for SMC-based communication
- **Platform-specific implementations** for AMD Versal2
- **TF-A custom package integration** with automatic build-time version tracking


## License

This project is licensed under BSD-3-Clause. See the LICENSE file in the repository root for full license text.

Copyright (c) 2023-2026, Advanced Micro Devices, Inc. All rights reserved.

## Features

### Supported SCMI Protocols

1. **Base Protocol (0x10)**
   - Protocol version discovery
   - Vendor/sub-vendor identification
   - Implementation version reporting
   - Protocol attributes and discovery

2. **Clock Protocol (0x14)**
   - Clock enable/disable control
   - Rate configuration and query
   - Platform clock enumeration

3. **Power Domain Protocol (0x11)**
   - Power state management
   - Domain configuration and control

4. **Reset Domain Protocol (0x16)**
   - Reset assertion/deassertion
   - Reset domain management

### Build-Time Version Tracking

The package automatically captures and embeds version information:
- Git branch name
- Commit date
- Commit hash
- Build Date & timestamp
- Package path

All version information is available as preprocessor defines and printed during build.

## Directory Structure

```
scmi-server/
├── custom_pkg.mk       # TF-A custom package makefile with version tracking
├── README.md           # This file
├── LICENSE             # BSD-3-Clause license
├── inc/                # SCMI driver header files
│   ├── base.h          # Base protocol definitions
│   ├── clock.h         # Clock protocol definitions
│   ├── common.h        # Common SCMI definitions
│   ├── power_domain.h  # Power domain protocol definitions
│   ├── reset_domain.h  # Reset domain protocol definitions
│   ├── sensor.h        # Sensor protocol definitions
│   └── platform/       # Platform-specific headers
│       ├── versal2_scmi.h      # Versal2 SCMI resource IDs
│       └── versal2_platform.h  # Versal2 platform definitions and SIP_SCMI FID
├── src/                # SCMI source files
│   ├── protocols/      # SCMI protocol implementations
│   │   ├── base.c      # Base protocol with vendor info
│   │   ├── clock.c     # Clock management protocol
│   │   ├── power_domain.c  # Power domain protocol
│   │   ├── reset_domain.c  # Reset domain protocol
│   │   └── sensor.c    # Sensor protocol (stub)
│   ├── transport/      # SCMI transport layer
│   │   ├── entry.c     # SCMI message dispatcher
│   │   └── smt.c       # Shared Memory Transport (SMT) implementation
│   └── platform/       # Platform-specific implementations
│       ├── scmi.c      # AMD Versal2 clock/reset/power implementations
│       └── custom_svc.c # TF-A custom package interface
├── lib/                # Reserved for libraries
└── binary/             # Reserved for binaries
```

## Build Instructions

### Prerequisites

1. Clone the TF-A repository:
```bash
git clone https://github.com/AMD-AECG-SSW-PUBLIC/arm-trusted-firmware.git
cd arm-trusted-firmware/
```

2. Clone the scmi-server repository:
```bash
git clone https://github.com/AMD-AECG-SSW-PUBLIC/scmi-server.git
```

### Building with SCMI Server

The scmi-server integrates with TF-A using the `CUSTOM_PKG_PATH` build variable.

#### Basic Build (SCMI Server Only)
```bash
make RESET_TO_BL31=1 PLAT=versal2 CONSOLE=pl011_1 \
     CUSTOM_PKG_PATH="../scmi-server" \
     bl31
```

### Build Output

When built, you'll see version information:

```
================================================================================
SCMI Server Package Information:
  Branch:        main
  Last Commit:   2025-11-28
  Version:       a1b2c3d
  Build Date:    2025-11-28
  Build Time:    14:30:45
  Package Path:  ../scmi-server
================================================================================
```

## Platform Integration

### SiP SMC Handler

The scmi-server integrates with TF-A's SiP (Silicon Provider) SMC handler in `plat/amd/versal2/sip_svc_setup.c`:

```c
        case SOC_SIP_SVC_CUSTOM:
        case SOC_SIP_SVC64_CUSTOM:
                return custom_smc_handler(smc_fid, x1, x2, x3, x4,
                                          cookie, handle, flags);
```

### SMT Buffer Configuration

The Shared Memory Transport buffer is configured in platform definitions:
- **Buffer Address**: `0x7b000000` (defined in `platform_def.h`)
- **Buffer Size**: 4KB
- **Transport**: SMC fast call with FID `0xC2002000`

### Memory Mapping

The SMT buffer must be mapped in both TF-A and client (TF-A Tests):
- TF-A: Added to platform memory map with `MT_RW | MT_NS | MT_EXECUTE_NEVER`
- Client: Mapped with appropriate attributes for shared access

## Platform Makefile Integration

In TF-A `plat/amd/versal2/platform.mk`, the scmi-server is integrated via the `CUSTOM_PKG_PATH` variable:

```makefile
ifdef CUSTOM_PKG_PATH
include plat/amd/common/custom_pkg.mk
else
BL31_SOURCES += plat/amd/common/custom_svc_stub.c
endif
```

When `CUSTOM_PKG_PATH` points to scmi-server, `plat/amd/common/custom_pkg.mk` includes the package's `custom_pkg.mk`, which automatically sets required build flags and includes all necessary source files.

## Architecture Overview

### Component Organization

The scmi-server is organized into three main layers:

1. **Protocols Layer** (`src/protocols/`)
   - SCMI protocol implementations (Base, Clock, Power Domain, Reset Domain, Sensor)
   - Protocol-agnostic message handling
   - Calls platform-specific implementations

2. **Transport Layer** (`src/transport/`)
   - Shared Memory Transport (SMT) implementation
   - Message entry point and routing
   - SMC fastcall handling

3. **Platform Layer** (`src/platform/`)
   - AMD Versal2 platform-specific SCMI implementations
   - TF-A custom package interface stubs
   - Hardware abstraction

### Integration with TF-A

The integration flow:

```
plat/amd/versal2/platform.mk:
├── CUSTOM_PKG_PATH defined?
    ├── Yes → include plat/amd/common/custom_pkg.mk
    │         ├── Validates custom_pkg.mk exists in CUSTOM_PKG_PATH
    │         ├── Includes CUSTOM_PKG_PATH/custom_pkg.mk
    │         │   └── scmi-server/custom_pkg.mk sets:
    │         │       • Adds SCMI source files to BL31_SOURCES
    │         │       • Adds include paths
    │         │       • Embeds versio and built date/time information
    │         └── Sets CUSTOM_PKG_INCLUDED=1
    └── No  → Includes plat/amd/common/custom_svc_stub.c
              (Stub implementation for custom services)
```

**Key Points:**
- `CUSTOM_PKG_PATH` can point to scmi-server or other custom packages
- No built-in SCMI driver configuration is set by scmi-server
- Platform integration is handled via the custom package framework

## SCMI Protocols Supported

This implementation supports the following SCMI protocols:

1. **Base Protocol** - Vendor/implementation discovery
2. **Clock Protocol** - Clock management (enable/disable, rate control)
3. **Power Domain Protocol** - Power state management
4. **Reset Domain Protocol** - Reset control
5. **Sensor Protocol** - Sensor reading and management

## Testing

### Key Implementation Details

1. **SMT Structure Compliance**: The implementation strictly follows the SCMI SMT specification with proper field offsets:
   - `reserved0` (0x00)
   - `status` (0x04) - FREE/BUSY/ERROR flags
   - `reserved1` (0x08) - 8 bytes
   - `flags` (0x10)
   - `length` (0x14) - message_header + payload size
   - `message_header` (0x18)
   - `payload` (0x1C+)

2. **Atomic Operations**: Uses atomic loads/stores for status and length fields to ensure thread-safe operation

3. **Memory Ordering**: Client uses memory barriers (`dmb sy`) to ensure write visibility before signaling message ready

4. **Channel Management**: Internal busy flag prevents concurrent access to SMT channel

## Platform-Specific Implementation

The `src/platform/scmi.c` file contains AMD Versal2-specific implementations:

### Vendor Information
- **Vendor**: "AMD"
- **Product**: "Versal Gen 2"
- **Implementation Version**: 0x00000000

### Platform Functions

Protocol handlers call these platform-specific functions:
- `plat_scmi_clock_*()` - Clock enable/disable, rate get/set
- `plat_scmi_pd_*()` - Power domain state management
- `plat_scmi_rstd_*()` - Reset assert/deassert operations

### Porting to Other Platforms

To port to a different platform:
1. Replace `src/platform/scmi.c` with your platform implementation
2. Update `inc/platform/` headers with your platform definitions
4. Configure SMT buffer address in your respective `xxxx_platform.h`
5. Add SMT buffer to platform memory map

## Troubleshooting

### Common Issues

1. **SMT structure mismatch**: Ensure client and server use identical SMT header layout (28 bytes)
2. **Memory mapping**: SMT buffer must be mapped in both TF-A and client
3. **Cache coherency**: Use memory barriers and appropriate cache attributes
4. **SMC routing**: Verify SiP handler is registered with correct FID range

### Debug Tips

- Enable NOTICE/VERBOSE logs in `smt.c` to trace message flow
- Verify SMT buffer address matches in all components
- Check SMC FID is correctly routed to SCMI handler
- Validate channel status transitions (FREE → BUSY → FREE)
