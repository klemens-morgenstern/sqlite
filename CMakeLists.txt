cmake_minimum_required(VERSION 3.12...3.20)

set(BOOST_SQLITE_VERSION 1)
if(BOOST_SUPERPROJECT_VERSION)
    set(BOOST_SQLITE_VERSION ${BOOST_SUPERPROJECT_VERSION})
endif()

project(boost_sqlite
        VERSION "${BOOST_SQLITE_VERSION}"
        DESCRIPTION "A sqlite C++ library"
        LANGUAGES CXX)

set(BOOST_SQLITE_IS_ROOT OFF)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(BOOST_SQLITE_IS_ROOT ON)
endif()

set(BOOST_SQLITE_SOURCES
    src/detail/exception.cpp
    src/backup.cpp
    src/blob.cpp
    src/connection.cpp
    src/error.cpp
    src/field.cpp
    src/json.cpp
    src/meta_data.cpp
    src/resultset.cpp
    src/row.cpp
    src/value.cpp
)

find_package(SQLite3 REQUIRED)

if (BOOST_SQLITE_IS_ROOT)
    if(NOT BOOST_SUPERPROJECT_VERSION)
        option(BOOST_SQLITE_INSTALL "Install boost::sqlite files" ON)
        option(BOOST_SQLITE_BUILD_TESTS "Build boost::sqlite tests" ON)
        option(BOOST_SQLITE_BUILD_EXAMPLES "Build boost::sqlite examples" ON)
    else()
        set(BOOST_SQLITE_BUILD_TESTS ${BUILD_TESTING})
    endif()

    find_package(Threads REQUIRED)
    find_package(Boost REQUIRED system json OPTIONAL_COMPONENTS url unit_test_framework)

endif()

add_library(boost_sqlite ${BOOST_SQLITE_SOURCES})
target_include_directories(boost_sqlite PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(boost_sqlite PUBLIC Boost::system Boost::json SQLite::SQLite3)
add_library(Boost::sqlite ALIAS boost_sqlite)

add_library(boost_sqlite_ext ${BOOST_SQLITE_SOURCES} src/ext.cpp)
target_include_directories(boost_sqlite_ext PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(boost_sqlite_ext PUBLIC Boost::system Boost::json SQLite::SQLite3)
target_compile_definitions(boost_sqlite_ext PUBLIC BOOST_SQLITE_COMPILE_EXTENSION=1)
set_property(TARGET boost_sqlite_ext PROPERTY POSITION_INDEPENDENT_CODE ON)
add_library(Boost::sqlite_ext ALIAS boost_sqlite_ext)

if (NOT BOOST_SQLITE_IS_ROOT)
    if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
        set(BOOST_SQLITE_BUILD_TESTS ON)
    endif()
else()
    if(NOT BOOST_SUPERPROJECT_VERSION)
        option(BOOST_SQLITE_INSTALL "Install boost::sqlite files" ON)
        option(BOOST_SQLITE_BUILD_TESTS "Build boost::sqlite tests" ON)
        option(BOOST_SQLITE_BUILD_EXAMPLES "Build boost::sqlite examples" ON)
    else()
        set(BOOST_SQLITE_BUILD_TESTS ${BUILD_TESTING})
    endif()

    find_package(Doxygen REQUIRED doxygen)

    set(DOXYGEN_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/doc")
    configure_file(doc/Doxyfile.in doc/Doxyfile @ONLY)

    add_custom_target(
            boost_sqlite_doc
            COMMAND ${DOXYGEN_EXECUTABLE} "${PROJECT_BINARY_DIR}/doc/Doxyfile"
            COMMENT "Building documentation using Doxygen"
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            VERBATIM
    )

endif()


if(BUILD_SHARED_LIBS)
    target_compile_definitions(boost_sqlite     PUBLIC BOOST_SQLITE_DYN_LINK=1)
    target_compile_definitions(boost_sqlite_ext PUBLIC BOOST_SQLITE_DYN_LINK=1)
else()
    target_compile_definitions(boost_sqlite     PUBLIC BOOST_SQLITE_STATIC_LINK=1)
    target_compile_definitions(boost_sqlite_ext PUBLIC BOOST_SQLITE_STATIC_LINK=1)
endif()

if(BOOST_SQLITE_INSTALL AND NOT BOOST_SUPERPROJECT_VERSION)
    install(TARGETS boost_sqlite
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            )
endif()

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()


if(BOOST_SQLITE_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()
