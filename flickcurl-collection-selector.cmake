# ---------------------------------------------------------------------------
# flickcurl collection selector
#
# This directory contains multiple flickcurl installations, one per build
# variant, each in its own subdirectory named after the vcpkg triplet
# (e.g. x86-windows-static-md, x64-windows).
#
# Point FLICKCURL_ROOT (or CMAKE_PREFIX_PATH) at THIS directory and call
# find_package(flickcurl REQUIRED). CMake finds this file, which:
#   1. derives the consuming project's requirements (architecture,
#      CRT linkage, shared/static libflickcurl)
#   2. scans the subdirectories' flickcurl-package-info.cmake
#   3. includes the configuration of the matching build
#
# If no subdirectory matches, this fails with a list of what each
# subdirectory offers. If you point FLICKCURL_ROOT at a subdirectory
# directly, that build's own flickcurl-config.cmake runs and performs the
# same checks with detailed diagnostics.
# ---------------------------------------------------------------------------

# ---- what the consuming project wants --------------------------------------
if(DEFINED CMAKE_C_SIZEOF_DATA_PTR)
    if(CMAKE_C_SIZEOF_DATA_PTR EQUAL 8)
        set(_fc_want_arch x64)
    else()
        set(_fc_want_arch x86)
    endif()
else()
    set(_fc_want_arch "")   # language not initialized yet; skip arch check
endif()

# Which libflickcurl flavor the consumer wants: shared (DLL) or static.
# FLICKCURL_USE_SHARED, not CMake's BUILD_SHARED_LIBS — that one governs a
# project's OWN libraries and should not be set just to use flickcurl.
if(FLICKCURL_USE_SHARED)
    set(_fc_want_shared ON)
else()
    set(_fc_want_shared OFF)
endif()

# Documented CMAKE_MSVC_RUNTIME_LIBRARY values: MultiThreaded / MultiThreadedDLL
# / MultiThreadedDebug / MultiThreadedDebugDLL (case-insensitive), optionally
# with a configuration genex suffix. Static CRT = value without "DLL".
if(MSVC)
    if(CMAKE_MSVC_RUNTIME_LIBRARY)
        string(TOUPPER "${CMAKE_MSVC_RUNTIME_LIBRARY}" _fc_crt)
        if(_fc_crt MATCHES "DLL")
            set(_fc_want_static_crt OFF)
        else()
            set(_fc_want_static_crt ON)
        endif()
    else()
        set(_fc_want_static_crt OFF)   # empty (MSVC default) means -MD
    endif()
else()
    set(_fc_want_static_crt OFF)
endif()

# ---- scan candidate subdirectories -----------------------------------------
file(GLOB _fc_candidate_dirs RELATIVE "${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_LIST_DIR}/*")
set(_fc_matches "")
set(_fc_descriptions "")
foreach(_dir ${_fc_candidate_dirs})
    if(IS_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/${_dir}"
       AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/${_dir}/share/flickcurl/flickcurl-package-info.cmake")
        include("${CMAKE_CURRENT_LIST_DIR}/${_dir}/share/flickcurl/flickcurl-package-info.cmake")
        set(_fc_ok TRUE)
        if(NOT _fc_want_arch STREQUAL "" AND NOT flickcurl_ARCH STREQUAL "${_fc_want_arch}")
            set(_fc_ok FALSE)
        endif()
        if(NOT flickcurl_SHARED STREQUAL "${_fc_want_shared}")
            set(_fc_ok FALSE)
        endif()
        if(NOT flickcurl_STATIC_CRT STREQUAL "${_fc_want_static_crt}")
            set(_fc_ok FALSE)
        endif()
        if(_fc_ok)
            list(APPEND _fc_matches "${_dir}")
        endif()
        list(APPEND _fc_descriptions
            "  ${_dir}: ${flickcurl_ARCH}, libflickcurl=${flickcurl_SHARED}, CRT=${flickcurl_STATIC_CRT}, curl=${flickcurl_CURL_TARGET}")
    endif()
endforeach()

# ---- select ------------------------------------------------------------------
if(NOT _fc_matches)
    message(FATAL_ERROR
        "No matching flickcurl build found in ${CMAKE_CURRENT_LIST_DIR}.\n"
        "  Project wants: arch=${_fc_want_arch} (empty=unknown), "
        "libflickcurl shared=${_fc_want_shared}, static CRT=${_fc_want_static_crt}\n"
        "  Available builds:\n${_fc_descriptions}\n"
        "  Build the missing flickcurl variant, or adjust the project settings "
        "(architecture / FLICKCURL_USE_SHARED / CMAKE_MSVC_RUNTIME_LIBRARY).")
endif()

list(GET _fc_matches 0 _fc_selected)
list(LENGTH _fc_matches _fc_n_matches)
if(_fc_n_matches GREATER 1)
    message(WARNING
        "Multiple flickcurl builds match; selected '${_fc_selected}'.\n"
        "  Matches: ${_fc_matches}")
endif()

# Hand over to the selected build's own configuration (which re-runs the
# checks with detailed diagnostics, including libcurl/libxml2 flavors, and
# leaves flickcurl_* metadata variables set to the selected build's values).
include("${CMAKE_CURRENT_LIST_DIR}/${_fc_selected}/share/flickcurl/flickcurl-config.cmake")

message(STATUS "flickcurl: selected build '${_fc_selected}' "
    "(arch=${flickcurl_ARCH}, shared=${flickcurl_SHARED}, CRT=${flickcurl_STATIC_CRT})")
