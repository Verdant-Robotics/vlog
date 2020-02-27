cmake_minimum_required(VERSION 3.12)

project(vlog)

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
    -Wno-gnu-zero-variadic-macro-arguments")

add_library(vlog SHARED vlog.cpp)
target_include_directories(vlog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS vlog DESTINATION lib)

add_executable(vlog_test vlog_test.cpp)
target_link_libraries(vlog_test PUBLIC vlog)
