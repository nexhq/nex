cmake_minimum_required(VERSION 3.10)
project(nex VERSION 1.2.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(STATIC_LINKING "Build with static linking" ON)

# Find libcurl
find_package(CURL REQUIRED)

# Source files
set(SOURCES
    src/main.c
    src/commands/install.c
    src/commands/run.c
    src/commands/update.c
    src/commands/remove.c
    src/commands/list.c
    src/commands/search.c
    src/commands/info.c
    src/commands/self_update.c
    src/http/client.c
    src/package/manager.c
    src/config/config.c
    src/utils/utils.c
    deps/cJSON/cJSON.c
)

# Create executable
add_executable(nex ${SOURCES})

# Include directories
target_include_directories(nex PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/deps/cJSON
    ${CURL_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(nex ${CURL_LIBRARIES})

# Windows-specific settings
if(WIN32)
    target_link_libraries(nex ws2_32 crypt32 wldap32)
    
    if(STATIC_LINKING)
        set_target_properties(nex PROPERTIES
            LINK_FLAGS "-static"
        )
    endif()
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(nex PRIVATE /W4)
else()
    target_compile_options(nex PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install
install(TARGETS nex DESTINATION bin)
