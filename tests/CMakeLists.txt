fetch_dependency(gtest "https://git.martian.ag/open-source/gtest.git" "master")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
include(AddValgrindTest)

macro(add_vlog_test test_name test_cpp)
  add_executable(${test_name} ${test_cpp})
  set_target_properties(${test_name} PROPERTIES CXX_STANDARD 20)
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
      -Wno-ctad-maybe-unsupported \
      -Wno-global-constructors \
      -Wno-double-promotion \
      -Wno-padded")
  target_link_libraries(${test_name} PUBLIC vlog gtest backward)
  if (COMMAND add_unit_test)
    add_unit_test(${test_name} ${test_name})
  else()
    add_test(${test_name} ${test_name})
  endif()
endmacro()

add_vlog_test(test_vlog test_vlog.cpp)

add_vlog_test(test_vlog_fatals test_vlog_fatals.cpp)

add_vlog_test(test_vlog_non_fatals test_vlog_non_fatals.cpp)
#add_valgrind_test(test_vlog_non_fatals ${CMAKE_CURRENT_SOURCE_DIR}/valgrind.suppressions )
