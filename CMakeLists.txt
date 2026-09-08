cmake_minimum_required(VERSION 3.25)

# ---------------------------------------------------------------------------
# Install (checked BEFORE project(): project() would otherwise set the
# CMAKE_INSTALL_PREFIX default, C:/Program Files/flickcurl)
# ---------------------------------------------------------------------------
# Installing is opt-in: only when the user explicitly passed
# -DCMAKE_INSTALL_PREFIX.
if(DEFINED CMAKE_INSTALL_PREFIX)
    set(FLICKCURL_INSTALL ON)
else()
    set(FLICKCURL_INSTALL OFF)
    message(STATUS "CMAKE_INSTALL_PREFIX not specified - install is disabled. "
        "Configure with -DCMAKE_INSTALL_PREFIX=<dir> to enable it.")
endif()

# ---------------------------------------------------------------------------
# Locate vcpkg BEFORE project() - toolchain must be set early
# ---------------------------------------------------------------------------
set(_vcpkg_root "")
if(DEFINED ENV{VCPKG_ROOT})
    set(_vcpkg_root "$ENV{VCPKG_ROOT}")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg")
    set(_vcpkg_root "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg")
    set(_vcpkg_root "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg")
endif()

if(_vcpkg_root AND EXISTS "${_vcpkg_root}/scripts/buildsystems/vcpkg.cmake")
    # Put vcpkg_installed inside the build directory so each build
    # carries its own dependency cache (cmake --build removes unused triplets)
    set(VCPKG_INSTALLED_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed" CACHE PATH "vcpkg installed packages directory" FORCE)
    # Detect triplet BEFORE setting the toolchain so vcpkg uses it
    set(platform "")
    if(CMAKE_GENERATOR_PLATFORM)
        set(platform "${CMAKE_GENERATOR_PLATFORM}")
    endif()
    set(_triplet "x64")
    if(platform STREQUAL "x64")
        set(_triplet "x64")
    elseif(platform STREQUAL "x86")
        set(_triplet "x86")
    elseif(platform STREQUAL "ARM64")
        set(_triplet "arm64")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_triplet "x64")
    else()
        set(_triplet "x86")
    endif()
    # vcpkg triplets:
    #   x64-windows.cmake            → CRT=dynamic  LIB=dynamic
    #   x64-windows-static.cmake     → CRT=static   LIB=static
    #   x64-windows-static-md.cmake  → CRT=dynamic  LIB=static
    #   x86-windows.cmake            → CRT=dynamic  LIB=dynamic
    #   x86-windows-static.cmake     → CRT=static   LIB=static (community/)
    #   x86-windows-static-md.cmake  → CRT=dynamic  LIB=static (community/)
    #   arm64-windows.cmake          → CRT=dynamic  LIB=dynamic
    #   arm64-windows-static-md.cmake→ CRT=dynamic  LIB=static
    #
    # We propagate BUILD_SHARED_LIBS to control dependency linkage so that
    # flickcurl and its dependencies always share the same linkage model.
    # This avoids CRT mismatches and reduces configuration surface.
    #   BUILD_SHARED_LIBS=ON → deps are DLLs  (triplet "-windows")
    #   BUILD_SHARED_LIBS=OFF → deps are static (triplet "-windows-static")
    # FLICKCURL_STATIC_CRT independently controls our CRT linkage:
    #   FLICKCURL_STATIC_CRT=OFF → /MD (dynamic CRT) — default
    #   FLICKCURL_STATIC_CRT=ON  → /MT (static CRT)
    #   Note: for static deps + static CRT, the triplet is "-windows-static"
    #   (already has static CRT), so no suffix change needed.
    #   For static deps + dynamic CRT, we add "-md" suffix → "-windows-static-md"
    #   For dynamic deps + static CRT → unsupported (no vcpkg triplet exists)
    if(BUILD_SHARED_LIBS)
        if(FLICKCURL_STATIC_CRT)
            message(FATAL_ERROR
                "BUILD_SHARED_LIBS=ON (dynamic deps) with FLICKCURL_STATIC_CRT=ON "
                "is not supported: vcpkg has no triplet for dynamic deps + static CRT.")
        else()
            set(_triplet "${_triplet}-windows")
        endif()
    else()
        if(FLICKCURL_STATIC_CRT)
            # Static deps + static CRT → triplet is already "-windows-static"
            # (no suffix change needed; the "-static" in the triplet name means
            #  static CRT, which is what we want here)
            set(_triplet "${_triplet}-windows-static")
        else()
            # Static deps + dynamic CRT → add "-md" suffix
            set(_triplet "${_triplet}-windows-static-md")
        endif()
    endif()
    set(VCPKG_TARGET_TRIPLET "${_triplet}" CACHE STRING "vcpkg target triplet" FORCE)

    set(CMAKE_TOOLCHAIN_FILE "${_vcpkg_root}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "vcpkg toolchain file" FORCE)
    set(FLICKCURL_VCPKG_ROOT "${_vcpkg_root}" CACHE PATH "vcpkg installation root" FORCE)
    set(FLICKCURL_USE_VCPKG ON)
    message(STATUS "vcpkg found at: ${_vcpkg_root}")
    message(STATUS "Auto-detected triplet: ${VCPKG_TARGET_TRIPLET}")
    unset(_triplet)
    unset(_vcpkg_root)
endif()

# ---------------------------------------------------------------------------
# Version info
# ---------------------------------------------------------------------------
set(FLICKCURL_VERSION_MAJOR 1)
set(FLICKCURL_VERSION_MINOR 27)
set(FLICKCURL_VERSION_PATCH 0)
set(FLICKCURL_VERSION "${FLICKCURL_VERSION_MAJOR}.${FLICKCURL_VERSION_MINOR}.${FLICKCURL_VERSION_PATCH}")
math(EXPR FLICKCURL_VERSION_DECIMAL "${FLICKCURL_VERSION_MAJOR} * 10000 + ${FLICKCURL_VERSION_MINOR} * 100 + ${FLICKCURL_VERSION_PATCH}")

project(flickcurl
    VERSION ${FLICKCURL_VERSION_MAJOR}.${FLICKCURL_VERSION_MINOR}.${FLICKCURL_VERSION_PATCH}
    LANGUAGES C
)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(BUILD_SHARED_LIBS "Build shared library (DLL) instead of static" OFF)
option(FLICKCURL_STATIC_CRT "Link against static CRT (/MT) instead of dynamic (/MD)" OFF)
option(FLICKCURL_OFFLINE "Enable offline mode (cached web responses)" OFF)
option(FLICKCURL_CAPTURE "Enable capture mode (save web responses)" OFF)

if(FLICKCURL_OFFLINE AND FLICKCURL_CAPTURE)
    message(FATAL_ERROR "Cannot enable both FLICKCURL_OFFLINE and FLICKCURL_CAPTURE")
endif()


# Set output directories so all artifacts go to <build-dir>/Release/ (or Debug/)
# Users should use -B build-<triplet> to keep different configs isolated:
#   cmake -B build-x86-windows-static -G "Visual Studio 18 2026" -A Win32 ...
#   cmake -B build-x64-windows -G "Visual Studio 18 2026" -A x64 ...
# Multi-config generators (Visual Studio, MinGW Makefiles) need config-specific vars
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/RelWithDebInfo")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/RelWithDebInfo")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/RelWithDebInfo")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/MinSizeRel")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/MinSizeRel")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/MinSizeRel")

# ---------------------------------------------------------------------------
# Build type default
# ---------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# ---------------------------------------------------------------------------
# CRT linking
# ---------------------------------------------------------------------------
# Documented CMAKE_MSVC_RUNTIME_LIBRARY values (CMP0091 is NEW via the
# 3.16 minimum version). The $<$<CONFIG:Debug>:Debug> suffix selects
# /MTd or /MDd for Debug builds, matching vcpkg's debug dependency builds.
# vcpkg's consumer toolchain does not set this variable (verified), so
# CRT linkage is our prerogative; only warn if the user explicitly passed
# -DCMAKE_MSVC_RUNTIME_LIBRARY with a different static/dynamic flavor.
if(MSVC)
    if(DEFINED CACHE{CMAKE_MSVC_RUNTIME_LIBRARY})
        set(_fc_user_crt "$CACHE{CMAKE_MSVC_RUNTIME_LIBRARY}")
    else()
        set(_fc_user_crt "")
    endif()
    if(FLICKCURL_STATIC_CRT)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        set(_fc_ours_static ON)
    else()
        # Genex is evaluated at any position in the value (verified):
        # Debug -> "MultiThreadedDebugDLL", other configs -> "MultiThreadedDLL".
        # (Appending the suffix after "MultiThreadedDLL" instead would yield the
        # unrecognized "MultiThreadedDLLDebug" and an empty RuntimeLibrary.)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
        set(_fc_ours_static OFF)
    endif()
    if(_fc_user_crt)
        string(TOUPPER "${_fc_user_crt}" _fc_user_crt)
        if(_fc_user_crt MATCHES "DLL")
            set(_fc_user_static OFF)
        else()
            set(_fc_user_static ON)
        endif()
        if(NOT _fc_user_static EQUAL _fc_ours_static)
            message(WARNING
                "CMAKE_MSVC_RUNTIME_LIBRARY was explicitly set to "
                "'${_fc_user_crt}' but this build uses static CRT=${_fc_ours_static} "
                "(FLICKCURL_STATIC_CRT=${FLICKCURL_STATIC_CRT}). The build value "
                "takes precedence; vcpkg dependencies built for a different CRT "
                "flavor may not link or run correctly.")
        endif()
    endif()
    unset(_fc_user_crt)
    unset(_fc_ours_static)
    unset(_fc_user_static)
endif()

# The CMake/Win32 build generates a trivial config.h in src/ carrying the
# version/package definitions and including win32_flickcurl_config.h.
if(WIN32)
    configure_file(config.h.cmake.win32.in ${CMAKE_CURRENT_SOURCE_DIR}/src/config.h @ONLY)
    add_compile_definitions(HAVE_CONFIG_H)
endif()
# ---------------------------------------------------------------------------
# getopt (part of this project, used by the utilities; not a release artifact.
# libmtwist sources are compiled into libflickcurl itself - see src/CMakeLists.txt)
# ---------------------------------------------------------------------------
if(WIN32)
    add_library(getopt STATIC
        getopt/getopt.c
    )

    target_include_directories(getopt
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/getopt>
            $<INSTALL_INTERFACE:include>
    )

    target_compile_definitions(getopt
        PRIVATE
            _CRT_SECURE_NO_WARNINGS
    )
endif()

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
# Resolution order:
#   1. vcpkg (if its toolchain was loaded) - the triplet supplies curl,
#      libxml2 and all transitive dependencies.
#   2. Standard CMake find modules (FindCURL, FindLibXml2).
#      -DCURL_ROOT / -DLIBXML2_ROOT are bridged into CMAKE_PREFIX_PATH so
#      plain install prefixes are discovered (verified: FindCURL/FindLibXml2
#      locate bare include/ + lib/ layouts without any CMake config or
#      pkg-config files).
#      -DZLIB_ROOT likewise: zlib is an optional transitive dependency of a
#      static libcurl. If found it is linked into flickcurl (harmless when
#      not actually needed); if not found the build proceeds as before.
#
# In case 2 the user supplies the library builds and is responsible for
# matching static/dynamic and CRT linkage with the current configuration.
#
# After this section the imported targets CURL::libcurl and LibXml2::LibXml2
# always exist (ZLIB::ZLIB exists when zlib was found);
# all subdirectories link against these names.

if(FLICKCURL_USE_VCPKG)
    find_package(CURL REQUIRED)
    find_package(LibXml2 REQUIRED)
    # zlib: optional transitive dependency of a static libcurl (also pulled
    # in via curl's find_dependency when present). QUIET: absence is fine.
    find_package(ZLIB QUIET)
else()
    # 2) standard find modules, with *_ROOT bridged into the search path
    foreach(_dep_root ${CURL_ROOT} ${LIBXML2_ROOT} ${ZLIB_ROOT})
        if(_dep_root AND NOT "${_dep_root}" IN_LIST CMAKE_PREFIX_PATH)
            list(PREPEND CMAKE_PREFIX_PATH "${_dep_root}")
        endif()
    endforeach()
    find_package(CURL QUIET)
    find_package(LibXml2 QUIET)
    find_package(ZLIB QUIET)

    if(NOT TARGET CURL::libcurl)
        message(FATAL_ERROR
            "libcurl not found. Set -DCURL_ROOT=<prefix> with the layout\n"
            "  <prefix>/include/curl/curl.h\n"
            "  <prefix>/lib/libcurl.lib  (or curl.lib)\n"
            "or build with vcpkg (set VCPKG_ROOT).")
    endif()
    if(NOT TARGET LibXml2::LibXml2)
        message(FATAL_ERROR
            "libxml2 not found. Set -DLIBXML2_ROOT=<prefix> with the layout\n"
            "  <prefix>/include/libxml/xpath.h  (or <prefix>/include/libxml2/libxml/xpath.h)\n"
            "  <prefix>/lib/libxml2.lib\n"
            "or build with vcpkg (set VCPKG_ROOT).")
    endif()
    message(STATUS "Dependencies resolved without vcpkg (find_package)")
endif()

# ---------------------------------------------------------------------------
# Transitive link requirements of the dependencies
# ---------------------------------------------------------------------------
# For static consumers (flickcurl-config --private-libs, Libs.private in the
# .pc files) we walk the dependency targets' link interfaces and collect
# their own libraries plus everything they require. FC_LIBS doubles as a
# "seen" set: a name is never added twice, which also terminates circular
# dependencies. The result is the same set of libraries the build system
# links libflickcurl against. Resolved per configuration because debug
# dependencies can have different names (libcurl-d.lib / zlibd.lib).
function(fc_resolve target cfg)
  string(TOUPPER "${cfg}" _suf)
  # The target's own library file, if it has one.
  get_target_property(_loc ${target} IMPORTED_LOCATION_${_suf})
  if(_loc MATCHES "NOTFOUND")
    get_target_property(_loc ${target} IMPORTED_LOCATION)
  endif()
  if(_loc MATCHES "NOTFOUND")
    get_target_property(_loc ${target} LOCATION)
  endif()
  if(NOT _loc MATCHES "NOTFOUND")
    get_filename_component(_base "${_loc}" NAME_WE)
    list(FIND FC_LIBS "-l${_base}" _idx)
    if(_idx EQUAL -1)
      list(APPEND FC_LIBS "-l${_base}")
    else()
      # Already in the list: duplicate or cycle - do not re-walk.
      return(PROPAGATE FC_LIBS)
    endif()
  endif()
  # Link interface: nested targets (recursed) and plain names.
  get_target_property(_iface ${target} INTERFACE_LINK_LIBRARIES)
  get_target_property(_ii ${target} IMPORTED_LINK_INTERFACE_LIBRARIES)
  foreach(_item ${_iface} ${_ii})
    if(_item MATCHES "-NOTFOUND")
      continue()
    endif()
    string(REGEX REPLACE "^\\$<LINK_ONLY:(.*)>$" "\\1" _item "${_item}")
    string(REGEX REPLACE "^\\$<LINK_INTERFACE_LIBRARIES:(.*)>$" "\\1" _item "${_item}")
    if(_item MATCHES "::")
      fc_resolve(${_item} ${cfg})
    elseif(_item MATCHES "^-")
      list(FIND FC_LIBS "${_item}" _idx)
      if(_idx EQUAL -1)
        list(APPEND FC_LIBS "${_item}")
      endif()
    else()
      list(FIND FC_LIBS "-l${_item}" _idx)
      if(_idx EQUAL -1)
        list(APPEND FC_LIBS "-l${_item}")
      endif()
    endif()
  endforeach()
  return(PROPAGATE FC_LIBS)
endfunction()

foreach(_fc_cfg RELEASE DEBUG)
    string(TOLOWER "${_fc_cfg}" _fc_cfg_lc)
    set(FC_LIBS "")
    fc_resolve(LibXml2::LibXml2 ${_fc_cfg_lc})
    fc_resolve(CURL::libcurl ${_fc_cfg_lc})
    if(TARGET ZLIB::ZLIB)
        fc_resolve(ZLIB::ZLIB ${_fc_cfg_lc})
    endif()
    string(REPLACE ";" " " FC_LIBS_${_fc_cfg} "${FC_LIBS}")
endforeach()

# ---------------------------------------------------------------------------
# Subdirectories
# ---------------------------------------------------------------------------
add_subdirectory(src)
add_subdirectory(utils)
add_subdirectory(examples)

# ---------------------------------------------------------------------------
# Installation (only when FLICKCURL_INSTALL is ON)
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

if(FLICKCURL_INSTALL)
# Install public headers
install(FILES src/flickcurl.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Generate and install pkg-config files (vcpkg layout):
#   Release: lib/pkgconfig/flickcurl.pc      (libdir = <prefix>/lib)
#   Debug:   debug/lib/pkgconfig/flickcurl.pc (libdir = <prefix>/debug/lib)
# Generated from the original autotools template; bcrypt and other private
# dependencies go into Libs.private, not Libs.
# Generate with Unix line endings (MSYS2/pkgconf chokes on CRLF).
# libdir/includedir are expressed relative to ${prefix} (resolved by
# pkg-config, or by the shell at runtime in flickcurl-config).
set(VERSION "${FLICKCURL_VERSION}")
set(PACKAGE "flickcurl")
set(prefix "${CMAKE_INSTALL_PREFIX}")
set(exec_prefix "${CMAKE_INSTALL_PREFIX}")
set(includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
# Standard pkg-config names: libcurl (libcurl.pc), libxml-2.0 (libxml-2.0.pc).
# Version constraints mirror the original autotools build.
# Leading space: the template line is "Requires:@PKG_CONFIG_REQUIRES@".
set(PKG_CONFIG_REQUIRES " libxml-2.0 >= 2.6.8, libcurl >= 7.10.0")

set(libdir "\${prefix}/${CMAKE_INSTALL_LIBDIR}")
set(LIBS "${FC_LIBS_RELEASE}")
configure_file(
    flickcurl.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/flickcurl.pc
    @ONLY
    NEWLINE_STYLE UNIX
)
set(libdir "\${prefix}/debug/${CMAKE_INSTALL_LIBDIR}")
set(LIBS "${FC_LIBS_DEBUG}")
configure_file(
    flickcurl.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/flickcurl-debug.pc
    @ONLY
    NEWLINE_STYLE UNIX
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/flickcurl.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/flickcurl-debug.pc
    DESTINATION debug/${CMAKE_INSTALL_LIBDIR}/pkgconfig
    RENAME flickcurl.pc
)

# Generate flickcurl-config script from the original autotools template
# (autotools variable names set above).
set(libdir "\${prefix}/${CMAKE_INSTALL_LIBDIR}")
set(LIBS "${FC_LIBS_RELEASE}")
configure_file(
    flickcurl-config.in
    ${CMAKE_CURRENT_BINARY_DIR}/flickcurl-config
    @ONLY
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/flickcurl-config
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
)

# Install example config
install(FILES
    flickcurl.conf.example
    DESTINATION ${CMAKE_INSTALL_DATADIR}/flickcurl
)

# Install man pages (if present)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/docs/flickcurl.1")
    install(FILES
        docs/flickcurl.1
        docs/flickcurl-config.1
        docs/flickrdf.1
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
    )
endif()

# Install documentation
install(FILES
    LICENSE-2.0.txt
    README.md
    AUTHORS
    COPYING
    COPYING.LIB
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)
endif()

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
message(STATUS "")
message(STATUS "Flickcurl build configuration:")
message(STATUS "  Version:        ${FLICKCURL_VERSION}")
message(STATUS "  Build type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "  Shared library: ${BUILD_SHARED_LIBS}")
message(STATUS "  Static CRT:     ${FLICKCURL_STATIC_CRT}")
message(STATUS "  Offline mode:   ${FLICKCURL_OFFLINE}")
message(STATUS "  Capture mode:   ${FLICKCURL_CAPTURE}")
message(STATUS "  Windows:        ${WIN32}")
message(STATUS "  MSVC:           ${MSVC}")
message(STATUS "  CMake:          ${CMAKE_VERSION}")
if(FLICKCURL_USE_VCPKG)
    message(STATUS "  vcpkg:          ${FLICKCURL_VCPKG_ROOT} (triplet: ${VCPKG_TARGET_TRIPLET})")
else()
    message(STATUS "  vcpkg:          not used (dependencies via find_package / CURL_ROOT / LIBXML2_ROOT)")
endif()
message(STATUS "")
