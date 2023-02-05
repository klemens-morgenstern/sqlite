file(GLOB ALL_TEST_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(boost_sqlite_tests ${ALL_TEST_FILES})
target_link_libraries(boost_sqlite_tests PUBLIC Boost::system SQLite::SQLite3 Boost::json Boost::sqlite)
target_compile_definitions(boost_sqlite_tests PUBLIC BOOST_SQLITE_SEPARATE_COMPILATION=1)


add_test(NAME boost_sqlite_tests COMMAND boost_sqlite_tests)

add_subdirectory(extension)