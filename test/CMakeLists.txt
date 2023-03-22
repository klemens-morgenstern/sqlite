file(GLOB ALL_TEST_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(boost_sqlite_tests ${ALL_TEST_FILES})
target_link_libraries(boost_sqlite_tests PUBLIC Boost::system SQLite::SQLite3 Boost::json Boost::sqlite)
target_compile_definitions(boost_sqlite_tests PUBLIC BOOST_SQLITE_SEPARATE_COMPILATION=1)

add_test(NAME boost_sqlite_tests COMMAND boost_sqlite_tests)

add_subdirectory(extension)


file(GLOB ALL_PUBLIC_HEADERS RELATIVE ${CMAKE_SOURCE_DIR}/include/ ${CMAKE_SOURCE_DIR}/include/boost/sqlite/*.hpp)
set(ALL_PUBLIC_HEADER_TESTS "")
foreach(HEADER ${ALL_PUBLIC_HEADERS})
    get_filename_component(NAME ${HEADER} NAME_WLE)
    set(FNAME "${CMAKE_CURRENT_BINARY_DIR}/test_include_${NAME}.cpp")
    file(GENERATE OUTPUT ${FNAME}
         CONTENT "#include <${HEADER}>")
    list(APPEND ALL_PUBLIC_HEADER_TESTS ${FNAME})
endforeach()

add_library(boost_sqlite_all_public_headers_test OBJECT ${ALL_PUBLIC_HEADER_TESTS})
target_link_libraries(boost_sqlite_all_public_headers_test PUBLIC Boost::headers)
