cmake_minimum_required(VERSION 3.13)

project(vlog)

option( ENABLE_BACKTRACE "Enable backtrace printing" TRUE )

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
    -Wno-global-constructors \
    -Wno-exit-time-destructors \
    -Wno-double-promotion \
    -Wno-undef \
    -Wno-padded \
    -Wno-shorten-64-to-32 \
    -Wno-old-style-cast \
    -Wno-sign-conversion \
    -Wno-conditional-uninitialized \
    -Wno-unused-parameter \
    -Wno-sign-conversion \
    -Wno-extra-semi-stmt \
    -Wno-weak-vtables \
    -Wno-disabled-macro-expansion \
    -Wno-zero-as-null-pointer-constant \
    -Wno-gnu-zero-variadic-macro-arguments")

add_library( vlog SHARED vlog.cpp )
target_include_directories( vlog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )

if ( ${ENABLE_BACKTRACE} )
  target_compile_definitions( vlog PRIVATE ENABLE_BACKTRACE=1 )
  target_sources( vlog PRIVATE callstack.cpp )

  find_path (DWARF_INCLUDE_DIR
    NAMES
      libdwarf.h dwarf.h
    PATHS
      /usr/include
      /usr/include/libdwarf
      /usr/local/include
      /usr/local/include/libdwarf
      /opt/local/include
      ENV CPATH)

  target_compile_definitions( vlog PRIVATE -DBACKWARD_HAS_DWARF=1 )
  target_include_directories( vlog SYSTEM PRIVATE ${DWARF_INCLUDE_DIR} )
  target_link_libraries( vlog PRIVATE dl elf dwarf )

endif()


install(TARGETS vlog DESTINATION lib)

add_executable(vlog_test vlog_test.cpp)
target_link_libraries(vlog_test PUBLIC vlog)
