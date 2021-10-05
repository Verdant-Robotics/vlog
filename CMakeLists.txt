cmake_minimum_required(VERSION 3.13)

project(vlog)

# need to force so tests run outside of CI
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build Type" FORCE)
endif()

option(ENABLE_BACKTRACE "Enable Backtrace Printing" TRUE)
option(ENABLE_VLOG_TESTS "Enable VLog Tests" OFF)

set(VLOG_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
  add_compile_options(-march=x86-64-v3)
else()
  add_compile_options(-mcpu=native)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  set(VLOG_MAIN_PROJECT TRUE)
else()
  set(VLOG_MAIN_PROJECT FALSE)
endif()

# Have the stb susbsystem separate because it does not support all the warnings
add_library(vlogstb STATIC stb/vlog_stbprint.cpp)
target_include_directories(vlogstb SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/stb)
target_compile_options(vlogstb PRIVATE -Wno-extra-semi-stmt
  -Wno-conditional-uninitialized -Wno-implicit-fallthrough)

add_library(vlog SHARED src/vlog.cpp)
target_include_directories(vlog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(vlog PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(vlog PRIVATE -Werror -Wall -Wextra -Weverything -Werror=return-type
  -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-ctad-maybe-unsupported
  -Wno-missing-noreturn -Wno-global-constructors -Wno-reserved-id-macro)
target_link_libraries(vlog PRIVATE vlogstb)

if(${ENABLE_BACKTRACE})
  target_compile_definitions(vlog PRIVATE ENABLE_BACKTRACE=1)
  target_sources(vlog PRIVATE src/callstack.cpp)

  find_path(DWARF_INCLUDE_DIR NAMES libdwarf.h dwarf.h
            PATHS /usr/include /usr/include/libdwarf /usr/local/include /usr/local/include/libdwarf
                  /opt/local/include ENV CPATH)

  target_compile_definitions(vlog PRIVATE -DBACKWARD_HAS_DWARF=1)
  target_include_directories(vlog SYSTEM PRIVATE ${DWARF_INCLUDE_DIR})
  target_link_libraries(vlog PRIVATE dl elf dwarf stdc++fs)

  add_library(backward INTERFACE)
  target_include_directories(backward INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
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
