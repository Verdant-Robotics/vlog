cmake_minimum_required(VERSION 3.13)

project(vlog)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)

include(FetchDependency)

fetch_dependency(stb "https://git.martian.ag/open-source/stb.git" "master")

# need to force so tests run outside of CI
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build Type" FORCE)
endif()

option(ENABLE_BACKTRACE "Enable Backtrace Printing" TRUE)
option(ENABLE_VLOG_TESTS "Enable VLog Tests" OFF)

set(VLOG_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
  add_compile_options(-march=native)
else()
  add_compile_options(-mcpu=native)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} \
    -Werror \
    -Wall \
    -Wextra \
    -Weverything \
    -Werror=return-type \
    -Wno-c++98-compat \
    -Wno-c++98-compat-pedantic \
    -Wno-ctad-maybe-unsupported \
    -Wno-missing-noreturn \
    -Wno-global-constructors")

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  set(VLOG_MAIN_PROJECT TRUE)
else()
  set(VLOG_MAIN_PROJECT FALSE)
endif()

add_library(vlog SHARED vlog.cpp)
target_include_directories(vlog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(${ENABLE_BACKTRACE})
  target_compile_definitions(vlog PRIVATE ENABLE_BACKTRACE=1)
  target_sources(vlog PRIVATE callstack.cpp)

  find_path(DWARF_INCLUDE_DIR NAMES libdwarf.h dwarf.h
            PATHS /usr/include /usr/include/libdwarf /usr/local/include /usr/local/include/libdwarf
                  /opt/local/include ENV CPATH)

  target_compile_definitions(vlog PRIVATE -DBACKWARD_HAS_DWARF=1)
  target_include_directories(vlog SYSTEM PRIVATE ${DWARF_INCLUDE_DIR})
  target_link_libraries(vlog PRIVATE dl elf dwarf stb stdc++fs)
endif()

if(${ENABLE_VLOG_TESTS} OR ${VLOG_MAIN_PROJECT})
  enable_testing()
  # add_test(test_lint ${VLOG_ROOT_DIR}/scripts/lint.py)
  add_subdirectory(tests)
endif()

install(TARGETS vlog DESTINATION lib)

add_custom_target(vlog-check COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure)
add_custom_target(vlog-lint ${VLOG_ROOT_DIR}/scripts/lint.py)
add_custom_target(vlog-fix ${VLOG_ROOT_DIR}/scripts/lint.py -i)

set(vlog_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "vlog_SOURCE_DIR")
