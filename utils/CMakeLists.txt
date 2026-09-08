# ============================================================================
# Utilities
# ============================================================================

# Common flags for all utilities
set(UTIL_CPPFLAGS
    -I${CMAKE_SOURCE_DIR}/src
    -DMTWIST_CONFIG
    -I${CMAKE_SOURCE_DIR}/libmtwist
)

# Common libraries (library target is libflickcurl)
set(UTIL_LIBS libflickcurl)
set(UTIL_INCLUDES
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/libmtwist
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    $<$<BOOL:${WIN32}>:${CMAKE_SOURCE_DIR}/getopt>
)

# getopt library (Windows only)
if(WIN32)
    list(APPEND UTIL_LIBS getopt)
    list(APPEND UTIL_INCLUDES ${CMAKE_SOURCE_DIR}/getopt)
endif()

# ============================================================================
# flickcurl (main CLI) - OUTPUT_NAME "flickcurl" so final binary is flickcurl.exe
# ============================================================================
add_executable(flickcurl
    flickcurl.c
    commands.c
    cmdline.c
)
set_target_properties(flickcurl PROPERTIES
    OUTPUT_NAME "flickcurl"
)
target_include_directories(flickcurl PRIVATE ${UTIL_INCLUDES})
target_compile_options(flickcurl PRIVATE ${UTIL_CPPFLAGS} -D_CRT_SECURE_NO_WARNINGS)
target_link_libraries(flickcurl PRIVATE ${UTIL_LIBS} CURL::libcurl LibXml2::LibXml2)

# ============================================================================
# flickrdf (RDF tool - uses raptor_fake stubs on Windows)
# ============================================================================
add_executable(flickrdf
    flickrdf.c
    raptor_fake.c
    cmdline.c
)
target_include_directories(flickrdf PRIVATE ${UTIL_INCLUDES})
target_compile_options(flickrdf PRIVATE ${UTIL_CPPFLAGS} -D_CRT_SECURE_NO_WARNINGS)
target_link_libraries(flickrdf PRIVATE ${UTIL_LIBS} CURL::libcurl LibXml2::LibXml2)

# ============================================================================
# Extra utilities
# ============================================================================

# codegen
add_executable(codegen
    codegen.c
    cmdline.c
)
target_include_directories(codegen PRIVATE ${UTIL_INCLUDES})
target_compile_options(codegen PRIVATE ${UTIL_CPPFLAGS} -D_CRT_SECURE_NO_WARNINGS)
target_link_libraries(codegen PRIVATE ${UTIL_LIBS} CURL::libcurl LibXml2::LibXml2)

# list-methods
add_executable(list-methods
    list-methods.c
    cmdline.c
)
target_include_directories(list-methods PRIVATE ${UTIL_INCLUDES})
target_compile_options(list-methods PRIVATE ${UTIL_CPPFLAGS} -D_CRT_SECURE_NO_WARNINGS)
target_link_libraries(list-methods PRIVATE ${UTIL_LIBS} CURL::libcurl LibXml2::LibXml2)

# mangen
add_executable(mangen
    mangen.c
    commands.c
    cmdline.c
)
target_include_directories(mangen PRIVATE ${UTIL_INCLUDES})
target_compile_options(mangen PRIVATE ${UTIL_CPPFLAGS} -D_CRT_SECURE_NO_WARNINGS)
target_link_libraries(mangen PRIVATE ${UTIL_LIBS} CURL::libcurl LibXml2::LibXml2)

# When building static library, define FLICKCURL_STATIC so flickcurl.h uses no dllimport/dllexport
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(flickcurl PRIVATE FLICKCURL_STATIC)
    target_compile_definitions(flickrdf PRIVATE FLICKCURL_STATIC)
    target_compile_definitions(codegen PRIVATE FLICKCURL_STATIC)
    target_compile_definitions(list-methods PRIVATE FLICKCURL_STATIC)
    target_compile_definitions(mangen PRIVATE FLICKCURL_STATIC)
endif()

# ============================================================================
# Installation
# ============================================================================
if(FLICKCURL_INSTALL)
install(TARGETS flickcurl flickrdf codegen list-methods mangen
    RUNTIME DESTINATION $<IF:$<CONFIG:Debug>,debug/${CMAKE_INSTALL_BINDIR},${CMAKE_INSTALL_BINDIR}>
)
endif()
