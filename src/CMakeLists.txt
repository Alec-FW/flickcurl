# ============================================================================
# libflickcurl - Main Flickcurl library
# ============================================================================
include(GNUInstallDirs)

# Source files (mirroring autotools Makefile.am)
set(FLICKCURL_SOURCES
    activity.c
    args.c
    blog.c
    category.c
    collection.c
    common.c
    comments.c
    contacts.c
    context.c
    config.c
    exif.c
    gallery.c
    group.c
    institution.c
    md5.c
    location.c
    machinetags.c
    members.c
    method.c
    note.c
    person.c
    photo.c
    photoset.c
    place.c
    serializer.c
    shape.c
    size.c
    stat.c
    ticket.c
    user_upload_status.c
    tags.c
    video.c
    vsnprintf.c
    win32_extras.c
    activity-api.c
    auth-api.c
    blogs-api.c
    commons-api.c
    collections-api.c
    contacts-api.c
    favorites-api.c
    interestingness-api.c
    galleries-api.c
    groups-api.c
    groups-members-api.c
    groups-pools-api.c
    machinetags-api.c
    people-api.c
    perms.c
    panda-api.c
    photos-api.c
    photos-comments-api.c
    photos-geo-api.c
    photos-licenses-api.c
    photos-notes-api.c
    photos-people-api.c
    photos-transform-api.c
    photos-upload-api.c
    photosets-api.c
    photosets-comments-api.c
    places-api.c
    prefs-api.c
    reflection-api.c
    stats-api.c
    tags-api.c
    test-api.c
    upload-api.c
    urls-api.c
    sha1.c
    legacy-auth.c
    oauth.c
    # libmtwist (submodule) - internal-only (declared in flickcurl_internal.h),
    # compiled into libflickcurl so static consumers don't need a separate
    # libmtwist.lib.
    ../libmtwist/mt.c
    ../libmtwist/seed.c
)

# MTWIST_CONFIG selects the flickcurl-specific mtwist configuration header
# (src/mtwist_config.h) - must stay scoped to the mtwist sources only.
set_source_files_properties(
    ../libmtwist/mt.c
    ../libmtwist/seed.c
    PROPERTIES COMPILE_DEFINITIONS "MTWIST_CONFIG"
)

# Internal headers (not installed)
set(FLICKCURL_HEADERS
    flickcurl_internal.h
    mtwist_config.h
    win32_flickcurl_config.h
)

# Library target (renamed to avoid conflict with vcpkg/CMake targets)
add_library(libflickcurl ${FLICKCURL_SOURCES} ${FLICKCURL_HEADERS})

# Public header
target_include_directories(libflickcurl
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libmtwist>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/getopt>
)

# Compile definitions
# FLICKCURL_INTERNAL — defined when building the library itself (dllexport)
# FLICKCURL_STATIC — defined when building/linking against a static library (no declspec)
# FLICKCURL_DLL — defined when the consumer is linking against a DLL (dllimport)
target_compile_definitions(libflickcurl
    PRIVATE
        MTWIST_CONFIG
        _CRT_SECURE_NO_WARNINGS
        $<$<BOOL:${FLICKCURL_OFFLINE}>:OFFLINE>
        $<$<BOOL:${FLICKCURL_CAPTURE}>:CAPTURE>
)
# FLICKCURL_STATIC must reach consumers of a static build (it switches
# FLICKCURL_API off in flickcurl.h), so it is PUBLIC: it applies to the
# library's own build AND is exported on the installed target.
if(BUILD_SHARED_LIBS)
    target_compile_definitions(libflickcurl PRIVATE FLICKCURL_INTERNAL=1 FLICKCURL_DLL)
else()
    target_compile_definitions(libflickcurl PUBLIC FLICKCURL_STATIC=1)
endif()

# Link dependencies
# BCryptGenRandom for getentropy in win32_extras.c
target_link_libraries(libflickcurl PRIVATE $<$<BOOL:${WIN32}>:bcrypt>)

# Link curl and libxml2 (vcpkg provides CMake targets)
# LibXml2 is PUBLIC: flickcurl.h exposes libxml2 types, so consumers need its
# include directories (and the library) whether they link the static or the
# shared libflickcurl. Curl stays PRIVATE: flickcurl.h exposes no curl types,
# so consumers only need it at link time (emitted as $<LINK_ONLY:>).
target_link_libraries(libflickcurl PUBLIC LibXml2::LibXml2)
target_link_libraries(libflickcurl PRIVATE CURL::libcurl)
# Optional transitive dependency of a static libcurl (found via ZLIB_ROOT /
# find_package(ZLIB QUIET) in the root CMakeLists.txt). Harmless when the
# linker doesn't pull anything from it.
if(TARGET ZLIB::ZLIB)
    target_link_libraries(libflickcurl PRIVATE ZLIB::ZLIB)
endif()

# Get include dirs from vcpkg targets and add to libflickcurl PUBLIC
get_target_property(_curl_inc CURL::libcurl INTERFACE_INCLUDE_DIRECTORIES)
# Remove possible duplicate paths (suspected pkg-config interference)
list(REMOVE_DUPLICATES _curl_inc)
if(_curl_inc)
    target_include_directories(libflickcurl PUBLIC $<BUILD_INTERFACE:${_curl_inc}>)
endif()

# DLL export/import handling
if(BUILD_SHARED_LIBS)
    set_target_properties(libflickcurl PROPERTIES
        VERSION 0.0.0
        SOVERSION 0
        PREFIX ""
        OUTPUT_NAME "flickcurl"
    )
    # Generate export macro for DLL
    target_compile_definitions(libflickcurl
        PUBLIC
            $<$<BOOL:${BUILD_SHARED_LIBS}>:FLICKCURL_DLL_EXPORT>
    )
else()
    set_target_properties(libflickcurl PROPERTIES
        PREFIX ""
        OUTPUT_NAME "libflickcurl"
        ARCHIVE_OUTPUT_NAME "libflickcurl"
    )
endif()

# Install library and public header.
# Mirror the vcpkg installed-tree layout so that Debug and Release
# artifacts do not overwrite each other: vcpkg puts Debug artifacts in a
# top-level debug/ subdirectory (debug/bin, debug/lib, debug/lib/pkgconfig),
# CMake package files in share/<pkg>/, and pkg-config files in
# lib/pkgconfig. install(TARGETS) takes at most one DESTINATION per artifact
# kind, so one call per configuration group is needed. The generated export
# files then reference debug/lib and debug/bin for the DEBUG imported
# configuration, exactly like the vcpkg-installed dependency packages do.
if(FLICKCURL_INSTALL)
if(CMAKE_CONFIGURATION_TYPES)
    # Multi-configuration generator (e.g. Visual Studio).
    # A single install(TARGETS) with per-configuration generator expressions
    # in DESTINATION: the generated export files record <prefix>/lib and
    # <prefix>/bin for the non-Debug imported configurations and
    # <prefix>/debug/lib and <prefix>/debug/bin for DEBUG.
    install(TARGETS libflickcurl
        EXPORT libflickcurl-targets
        LIBRARY DESTINATION $<IF:$<CONFIG:Debug>,debug/${CMAKE_INSTALL_LIBDIR},${CMAKE_INSTALL_LIBDIR}>
            COMPONENT Runtime
        ARCHIVE DESTINATION $<IF:$<CONFIG:Debug>,debug/${CMAKE_INSTALL_LIBDIR},${CMAKE_INSTALL_LIBDIR}>
            COMPONENT Development
        RUNTIME DESTINATION $<IF:$<CONFIG:Debug>,debug/${CMAKE_INSTALL_BINDIR},${CMAKE_INSTALL_BINDIR}>
            COMPONENT Runtime
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
else()
    # Single-configuration generator
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_fc_install_subdir debug/)
    else()
        set(_fc_install_subdir "")
    endif()
    install(TARGETS libflickcurl
        EXPORT libflickcurl-targets
        LIBRARY DESTINATION ${_fc_install_subdir}${CMAKE_INSTALL_LIBDIR}
            COMPONENT Runtime
        ARCHIVE DESTINATION ${_fc_install_subdir}${CMAKE_INSTALL_LIBDIR}
            COMPONENT Development
        RUNTIME DESTINATION ${_fc_install_subdir}${CMAKE_INSTALL_BINDIR}
            COMPONENT Runtime
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/flickcurl.h" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" COMPONENT Development)

# CMake package files go to share/flickcurl/ (vcpkg convention for CMake
# package files); the standard find_package() config search mode covers it.
install(EXPORT libflickcurl-targets
    FILE libflickcurl-targets.cmake
    NAMESPACE flickcurl::
    DESTINATION ${CMAKE_INSTALL_DATADIR}/flickcurl
    COMPONENT Development
)
endif() # multi/single-config install(TARGETS) block - see FLICKCURL_INSTALL below

# ---------------------------------------------------------------------------
# CMake package files: find_package(flickcurl) support + consumer checks.
# Metadata (arch, CRT, dependency flavors) is recorded at install time so a
# consuming project can verify it is linking a compatible build.
# ---------------------------------------------------------------------------
if(CMAKE_C_SIZEOF_DATA_PTR EQUAL 8)
    set(FLICKCURL_ARCH "x64")
else()
    set(FLICKCURL_ARCH "x86")
endif()
# The canonical libcurl target name is what gets baked into the export file:
# vcpkg static triplets expose CURL::libcurl_static, everything else CURL::libcurl.
if(TARGET CURL::libcurl_static)
    set(FLICKCURL_CURL_TARGET "CURL::libcurl_static")
else()
    set(FLICKCURL_CURL_TARGET "CURL::libcurl")
endif()
get_target_property(FLICKCURL_LIBXML2_TYPE LibXml2::LibXml2 TYPE)
if(NOT FLICKCURL_LIBXML2_TYPE)
    set(FLICKCURL_LIBXML2_TYPE "")
endif()
if(TARGET ZLIB::ZLIB)
    set(FLICKCURL_DEPENDS_ZLIB ON)
else()
    set(FLICKCURL_DEPENDS_ZLIB OFF)
endif()
if(FLICKCURL_STATIC_CRT)
    set(FLICKCURL_CRT_TEXT "static CRT (-MT)")
else()
    set(FLICKCURL_CRT_TEXT "dynamic CRT (-MD)")
endif()
set(FLICKCURL_SHARED ${BUILD_SHARED_LIBS})

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../flickcurl-package-info.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/flickcurl-package-info.cmake
    @ONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../flickcurl-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/flickcurl-config.cmake
    @ONLY
)
if(FLICKCURL_INSTALL)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/flickcurl-package-info.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/flickcurl-config.cmake
    DESTINATION ${CMAKE_INSTALL_DATADIR}/flickcurl
    COMPONENT Development
)
endif()

# ============================================================================
# Test executables
# ============================================================================
# NOTE: These 4 test executables link against internal library functions
# (flickcurl_init_params, flickcurl_end_params, flickcurl_prepare_noauth,
#  flickcurl_oauth_build_key, flickcurl_oauth_compute_signature,
#  flickcurl_build_person) that are declared in flickcurl_internal.h
# but not exported via FLICKCURL_API in flickcurl.h.
#
# In a DLL build these symbols are not exported, so the tests cannot link
# against the library.  They are only built for static builds where the
# entire library is inlined into the consumer.
#
# The autotools Makefile.am had the same pattern — these tests always
# linked libflickcurl.la.  On Unix a shared library exports all symbols
# by default so the problem was hidden.  On Windows the DLL model
# requires explicit __declspec(dllexport) which these functions lack.
# ============================================================================

# flickcurl_oauth_test
if(NOT BUILD_SHARED_LIBS)
add_executable(flickcurl_oauth_test
    ${CMAKE_CURRENT_SOURCE_DIR}/oauth.c
)
target_include_directories(flickcurl_oauth_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/libmtwist
    $<$<BOOL:${WIN32}>:${CMAKE_SOURCE_DIR}/getopt>
)
target_compile_definitions(flickcurl_oauth_test PRIVATE
    MTWIST_CONFIG
    _CRT_SECURE_NO_WARNINGS
    STANDALONE
)
target_link_libraries(flickcurl_oauth_test PRIVATE libflickcurl CURL::libcurl LibXml2::LibXml2)
endif()

# config_test
if(NOT BUILD_SHARED_LIBS)
add_executable(config_test
    ${CMAKE_CURRENT_SOURCE_DIR}/config_test.c
)
target_include_directories(config_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/libmtwist
    $<$<BOOL:${WIN32}>:${CMAKE_SOURCE_DIR}/getopt>
)
target_compile_definitions(config_test PRIVATE
    MTWIST_CONFIG
    _CRT_SECURE_NO_WARNINGS
)
target_link_libraries(config_test PRIVATE libflickcurl CURL::libcurl LibXml2::LibXml2)
endif()

# person_test
if(NOT BUILD_SHARED_LIBS)
add_executable(person_test
    ${CMAKE_CURRENT_SOURCE_DIR}/person_test.c
)
target_include_directories(person_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/libmtwist
    $<$<BOOL:${WIN32}>:${CMAKE_SOURCE_DIR}/getopt>
)
target_compile_definitions(person_test PRIVATE
    MTWIST_CONFIG
    _CRT_SECURE_NO_WARNINGS
)
target_link_libraries(person_test PRIVATE libflickcurl CURL::libcurl LibXml2::LibXml2)
endif()

# oauth_sign_test
if(NOT BUILD_SHARED_LIBS)
add_executable(oauth_sign_test
    ${CMAKE_CURRENT_SOURCE_DIR}/oauth_sign_test.c
)
target_include_directories(oauth_sign_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/libmtwist
    $<$<BOOL:${WIN32}>:${CMAKE_SOURCE_DIR}/getopt>
)
target_compile_definitions(oauth_sign_test PRIVATE
    MTWIST_CONFIG
    _CRT_SECURE_NO_WARNINGS
)
target_link_libraries(oauth_sign_test PRIVATE libflickcurl CURL::libcurl LibXml2::LibXml2)
endif()

# Add tests to ctest (only when they exist)
if(NOT BUILD_SHARED_LIBS)
add_test(NAME flickcurl_oauth_test COMMAND flickcurl_oauth_test)
add_test(NAME config_test COMMAND config_test)
add_test(NAME person_test COMMAND person_test)
add_test(NAME oauth_sign_test COMMAND oauth_sign_test)
endif()

