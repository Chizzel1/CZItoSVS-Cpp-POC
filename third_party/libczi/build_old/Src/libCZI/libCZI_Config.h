// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

// those numbers define the version of libCZI
#define LIBCZI_VERSION_MAJOR "0"
#define LIBCZI_VERSION_MINOR "67"
#define LIBCZI_VERSION_PATCH "2"
#define LIBCZI_VERSION_TWEAK ""

// whether the libCZI library is built with support for the Windows-API, i.e. whether it is being built for a Windows-environment
#define LIBCZI_WINDOWSAPI_AVAILABLE 1

// whether the libCZI library is built with support for the Windows-UWP-API, i.e. whether it is being built for an UWP-environment
#define LIBCZI_WINDOWS_UWPAPI_AVAILABLE 0

// if the host system is a big-endian system, this is "1", otherwise 0
#define LIBCZI_ISBIGENDIANHOST 0

// if the include-file "endian.h" is available, then this is "1", otherwise 0
#define LIBCZI_HAVE_ENDIAN_H 0

// whether the processor can load integers from an unaligned address, if this
//  is 1 it means that we cannot load an integer from an unaligned address (we'd
//  get a bus-error if we try), 0 means that the CPU can load from unaligned addresses
#define LIBCZI_SIGBUS_ON_UNALIGNEDINTEGERS 0

#define LIBCZI_CXX_COMPILER_IDENTIFICATION "MSVC 19.44.35217.0"

// whether the function "aligned_alloc" is available
#define LIBCZI_HAVE_ALIGNED_ALLOC 0

// whether the function "_aligned_malloc" is available
#define LIBCZI_HAVE__ALIGNED_MALLOC 1

// whether we can use pread/pwrite-APIs (for implementing file-stream objects), only relevant if not Win32-environment
#define LIBCZI_USE_PREADPWRITEBASED_STREAMIMPL 0

#define LIBCZI_REPOSITORYREMOTEURL "https://github.com/zeiss/libczi"

#define LIBCZI_REPOSITORYBRANCH    "main"

#define LIBCZI_REPOSITORYHASH      "2d6e9ac7b320373b099d55c8ebe0ac0cf16bb0da"

// whether the header "immintrin.h" is available and AVX-SIMD intrinsics can be used
#define LIBCZI_HAS_AVXINTRINSICS   1

// whether ARM-Neon-intrinsics can be used
#define LIBCZI_HAS_NEOININTRINSICS 0

// whether the curl-based stream implementations are available (and whether libCZI can use the libcurl library)
#define LIBCZI_CURL_BASED_STREAM_AVAILABLE  0

// whether the Azure-SDK stream implementation is available (and whether libCZI can use the Azure-Storage library)
#define LIBCZI_AZURESDK_BASED_STREAM_AVAILABLE  0

#define LIBCZI_AZURESDK_VERSION_INFO "not available"

// whether the intrinsic "__builtin_bswap32" is available (in the header-file byteswap.h)
#define LIBCZI_HAS_BUILTIN_BSWAP32 0

// whether the function "_byteswap_ulong" is available (in the header-file stdlib.h)
#define LIBCZI_HAS_BYTESWAP_IN_STDLIB 1

// whether the function "bswap32" is available (in the header-file sys/endian.h)
#define LIBCZI_HAS_BSWAP_LONG_IN_SYS_ENDIAN 0
