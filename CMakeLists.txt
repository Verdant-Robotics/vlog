cmake_minimum_required(VERSION 3.13)

project(vlog)

# Tests assume debug symbols are present
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build Type" FORCE)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  option(ENABLE_BACKTRACE "Enable Backtrace Printing" TRUE)
else()
  option(ENABLE_BACKTRACE "Enable Backtrace Printing" FALSE)
endif()

option(ENABLE_VLOG_TESTS "Enable VLog Tests" ON)

set(VLOG_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
  add_compile_options(-march=x86-64-v3)
else()
  add_compile_options(-mcpu=native)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  set(VLOG_MAIN_PROJECT TRUE)
else()
  set(VLOG_MAIN_PROJECT FALSE)
endif()
message("source dir ${CMAKE_SOURCE_DIR}  current ${CMAKE_CURRENT_SOURCE_DIR}")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)
include(FetchDependency)

# Have the stb susbsystem separate because it does not support all the warnings
add_library(vlogstb STATIC stb/vlog_stbprint.cpp)
target_include_directories(vlogstb SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/stb)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
    "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    target_compile_options(vlogstb PRIVATE
        -Wno-extra-semi-stmt -Wno-conditional-uninitialized -Wno-implicit-fallthrough)
endif()
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "14.0.0")
  # on new compiler version, we need to disable warning
  target_compile_options(vlogstb PRIVATE -Wno-reserved-identifier)
endif()

add_library(vlog SHARED src/vlog.cpp)
target_include_directories(vlog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(vlog PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  target_compile_options(vlog PRIVATE -pedantic -Werror -Wall -Wextra -Wno-stringop-truncation)
else ()
  target_compile_options(vlog PRIVATE  -Weverything -Werror -Wall -Wextra  -Werror=return-type
          -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-ctad-maybe-unsupported
          -Wno-missing-noreturn -Wno-global-constructors -Wno-reserved-id-macro)
endif()
target_compile_options(vlog PUBLIC -pthread)
target_link_libraries(vlog PRIVATE vlogstb)
target_link_libraries(vlog PUBLIC pthread)

if(${ENABLE_BACKTRACE})
  find_package(LibElf REQUIRED)
  find_package(LibDwarf REQUIRED)

  target_compile_definitions(vlog PRIVATE ENABLE_BACKTRACE=1)
  target_compile_definitions(vlog PRIVATE -DBACKWARD_HAS_DWARF=1)

  target_sources(vlog PRIVATE src/callstack.cpp)
  target_link_libraries(vlog PRIVATE dl ${LIBELF_LIBRARIES} ${LIBDWARF_LIBRARIES})

  add_library(backward INTERFACE)
  target_include_directories(backward INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
else()
  target_compile_definitions(vlog PRIVATE ENABLE_BACKTRACE=0)
endif()

if(${ENABLE_VLOG_TESTS} OR ${VLOG_MAIN_PROJECT})
  enable_testing()
  add_subdirectory(tests)
endif()

install(TARGETS vlog DESTINATION lib)

add_custom_target(vlog-check COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure)
add_custom_target(vlog-lint ${VLOG_ROOT_DIR}/scripts/lint.py)
add_custom_target(vlog-fix ${VLOG_ROOT_DIR}/scripts/lint.py -i)

set(vlog_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "vlog_SOURCE_DIR")
