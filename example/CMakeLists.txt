
file(GLOB_RECURSE ALL_EXAMPLES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE ALL_EXAMPLES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

foreach(SRC ${ALL_EXAMPLES})
    get_filename_component(NAME ${SRC} NAME_WLE )

    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.sql)
        add_library(boost_sqlite_example_${NAME} SHARED ${SRC})
        target_link_libraries(boost_sqlite_example_${NAME} PUBLIC Boost::sqlite_ext Boost::url)
        target_compile_definitions(boost_sqlite_example_${NAME} PUBLIC BOOST_SQLITE_SEPARATE_COMPILATION=1)
        set_property(TARGET boost_sqlite_example_${NAME} PROPERTY PREFIX "")
        set_target_properties(boost_sqlite_example_${NAME} PROPERTIES OUTPUT_NAME ${NAME} POSITION_INDEPENDENT_CODE ON)
     else()
        add_executable(boost_sqlite_example_${NAME} ${SRC} )
        target_link_libraries(boost_sqlite_example_${NAME} PUBLIC Boost::sqlite)
        target_compile_definitions(boost_sqlite_example_${NAME} PUBLIC BOOST_SQLITE_SEPARATE_COMPILATION=1)
    endif()
endforeach()

