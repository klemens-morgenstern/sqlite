
file(GLOB_RECURSE ALL_EXAMPLES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

foreach(SRC ${ALL_EXAMPLES})
    get_filename_component(NAME ${SRC} NAME_WLE )
    add_executable(boost_sqlite_example_${NAME} ${SRC} )

    target_link_libraries(boost_sqlite_example_${NAME} PUBLIC Boost::sqlite)
    target_compile_definitions(boost_sqlite_example_${NAME} PUBLIC BOOST_SQLITE_SEPARATE_COMPILATION=1)
endforeach()
