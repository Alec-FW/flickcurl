# getopt library (Windows only - MSVC lacks getopt)
# Built as a static library to provide getopt/getopt_long on Windows

add_library(getopt STATIC
    getopt.c
)

target_include_directories(getopt
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_compile_definitions(getopt
    PRIVATE
        _CRT_SECURE_NO_WARNINGS
)

# Install public header
if(FLICKCURL_INSTALL)
install(FILES
    flickcurl_getopt.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/getopt
)
endif()
