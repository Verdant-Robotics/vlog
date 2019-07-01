project(vlog)

add_library( vlog SHARED vlog.cpp )
target_include_directories( vlog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )
install(TARGETS vlog DESTINATION lib)

add_executable( vlog_test vlog_test.cpp )
target_link_libraries( vlog_test PUBLIC vlog )
