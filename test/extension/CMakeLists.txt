
file(GLOB_RECURSE ALL_SQLITE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.sql)
file(GLOB_RECURSE ALL_CPP_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

foreach(module ${ALL_CPP_FILES})
    get_filename_component(stem ${module} NAME_WE)
    add_library(${stem} SHARED ${module})
    target_link_libraries(${stem} PUBLIC Boost::system Boost::sqlite_ext)
    target_include_directories(${stem} PUBLIC ../../include)
    set_property(TARGET ${stem} PROPERTY PREFIX "")
endforeach()



foreach(script ${ALL_SQLITE_FILES})
    get_filename_component(stem ${script} NAME_WE)
    add_test(NAME ${stem} COMMAND
            sqlite3 :memory: ".read ${CMAKE_CURRENT_SOURCE_DIR}/${script}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endforeach()