
file(GLOB_RECURSE ALL_SQLITE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.sql)
file(GLOB_RECURSE ALL_CPP_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

foreach(module ${ALL_CPP_FILES})
    get_filename_component(stem ${module} NAME_WE)
    add_library(boost_sqlite_test_extension_${stem} SHARED ${module})
    target_link_libraries(boost_sqlite_test_extension_${stem} PUBLIC Boost::sqlite_ext)
    target_include_directories(boost_sqlite_test_extension_${stem} PUBLIC ../../include)
    set_property(TARGET boost_sqlite_test_extension_${stem} PROPERTY PREFIX "")
    set_target_properties(boost_sqlite_test_extension_${stem} PROPERTIES OUTPUT_NAME ${stem})
endforeach()

foreach(script ${ALL_SQLITE_FILES})
    get_filename_component(stem ${script} NAME_WE)
    add_test(NAME boost_sqlite_test_extension_${stem} COMMAND
            sqlite3 :memory: ".read ${CMAKE_CURRENT_SOURCE_DIR}/${script}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endforeach()
