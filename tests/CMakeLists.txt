find_package(GTest REQUIRED)

macro(add_vlog_test test_name test_cpp)
  add_executable(${test_name} ${test_cpp})
  set_target_properties(${test_name} PROPERTIES CXX_STANDARD 17)
  set_target_properties(${test_name} PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)
  set_target_properties(${test_name} PROPERTIES CMAKE_POSITION_INDEPENDENT_CODE ON)
  set_target_properties(
    ${test_name}
    PROPERTIES
      CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} \
      -Werror \
      -Wall \
      -Wextra \
      -Weverything \
      -Werror=return-type \
      -Wno-c++98-compat \
      -Wno-c++98-compat-pedantic \
      -Wno-global-constructors \
      -Wno-double-promotion \
      -Wno-padded")
  target_link_libraries(${test_name} PUBLIC vlog gtest)
  add_test(${test_name} ${test_name})
endmacro()

add_vlog_test(test_vlog test_vlog.cpp)
