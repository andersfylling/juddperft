# settings
set(CMAKE_VERBOSE_MAKEFILE ON)
cmake_minimum_required(VERSION 3.8)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # support for some linters

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

# In case neither release nor debug is set as build type, default to release.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE EQUAL "Release")
    set(CMAKE_CXX_FLAGS "-Wall -fPIC") # -Wno-unused-variable  -Wextra")
endif()

set(CMAKE_CXX_FLAGS "-Wall -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto -DNDEBUG")

# link headers
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

# Our source code
add_subdirectory(engine)
