# ============================================================================
# Examples
# ============================================================================

# Common flags
set(EXAMPLE_CPPFLAGS -I${CMAKE_SOURCE_DIR}/src)
set(EXAMPLE_INCLUDES
    ${CMAKE_SOURCE_DIR}/src
    $<$<BOOL:${WIN32}>:${CMAKE_SOURCE_DIR}/getopt>
)
set(EXAMPLE_LIBS libflickcurl)

if(WIN32)
    list(APPEND EXAMPLE_LIBS getopt)
    list(APPEND EXAMPLE_INCLUDES ${CMAKE_SOURCE_DIR}/getopt)
endif()

# ============================================================================
# print-photo-info
# ============================================================================
add_executable(print-photo-info
    print-photo-info.c
)
target_include_directories(print-photo-info PRIVATE ${EXAMPLE_INCLUDES})
target_compile_options(print-photo-info PRIVATE ${EXAMPLE_CPPFLAGS} -D_CRT_SECURE_NO_WARNINGS)
target_link_libraries(print-photo-info PRIVATE ${EXAMPLE_LIBS})
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(print-photo-info PRIVATE FLICKCURL_STATIC)
endif()

# ============================================================================
# search-photos
# ============================================================================
add_executable(search-photos
    search-photos.c
)
target_include_directories(search-photos PRIVATE ${EXAMPLE_INCLUDES})
target_compile_options(search-photos PRIVATE ${EXAMPLE_CPPFLAGS} -D_CRT_SECURE_NO_WARNINGS)
target_link_libraries(search-photos PRIVATE ${EXAMPLE_LIBS})
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(search-photos PRIVATE FLICKCURL_STATIC)
endif()
