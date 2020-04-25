cmake_minimum_required(VERSION 3.13)

project(vlog)

option(ENABLE_BACKTRACE "Enable Backtrace Printing" TRUE)
option(ENABLE_VLOG_TESTS "Enable VLog Tests" OFF)

set(VLOG_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

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
  target_link_libraries(vlog PRIVATE dl elf dwarf)
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
