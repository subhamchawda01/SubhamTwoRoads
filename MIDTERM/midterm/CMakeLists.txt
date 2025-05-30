cmake_minimum_required (VERSION 3.2)
project (midterm VERSION 1.0)

set(SHARED_LIBS OFF)
set(STATIC_LIBS ON)

# Do the release build by default
# For debug builds, please use: 'cmake .. -DCMAKE_BUILD_TYPE=Debug'
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Warnings and debug/release modes
set(CMAKE_CXX_FLAGS "-Wall -fdiagnostics-color=always")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# C++14 standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include qlservice directory by default
# Our entire codebase assumes the include path from the base directory
include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/..")
include_directories("${PROJECT_SOURCE_DIR}/../midterm-infra")

add_subdirectory(MidTermCode)
add_subdirectory(Execs)
