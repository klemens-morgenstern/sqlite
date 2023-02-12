cmake_minimum_required(VERSION 3.20)

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

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../boost.css)
    set(BOOST_SQLITE_SHOULD_BE_INLINE ON)
else()
    set(BOOST_SQLITE_SHOULD_BE_INLINE OFF)
endif()

option(BOOST_SQLITE_BUILD_INLINE "Configure as if part of the boost source tree" ${BOOST_SQLITE_SHOULD_BE_INLINE})

if(NOT BOOST_SUPERPROJECT_VERSION)
    option(BOOST_SQLITE_INSTALL "Install boost::sqlite files" ON)
    option(BOOST_SQLITE_BUILD_TESTS "Build boost::sqlite tests" ON)
    option(BOOST_SQLITE_BUILD_EXAMPLES "Build boost::sqlite examples" ON)
else()
    set(BOOST_SQLITE_BUILD_TESTS ${BUILD_TESTING})
endif()


if(BOOST_SQLITE_IS_ROOT AND BOOST_SQLITE_BUILD_INLINE)
    #
    # Building inside Boost tree, but as a separate project e.g. on Travis or
    # other CI, or when producing Visual Studio Solution and Projects.

    set(BOOST_INCLUDE_LIBRARIES SQLITE)
    set(BOOST_EXCLUDE_LIBRARIES SQLITE)

    set(CMAKE_FOLDER _deps)
    add_subdirectory(../.. _deps/boost EXCLUDE_FROM_ALL)
    unset(CMAKE_FOLDER)
endif()

if (NOT BOOST_SQLITE_BUILD_INLINE)
    find_package(Threads REQUIRED)
    find_package(Boost REQUIRED system json)
    include_directories(include)
endif()


find_package(Doxygen REQUIRED doxygen)

set(DOXYGEN_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/doc")
configure_file(doc/Doxyfile.in doc/Doxyfile @ONLY)

add_custom_target(
        doc
        COMMAND doxygen "${PROJECT_BINARY_DIR}/doc/Doxyfile"
        COMMENT "Building documentation using Doxygen"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        VERBATIM
)

find_package(SQLite3 REQUIRED)

add_library(boost_sqlite src/src.cpp)
target_include_directories(boost_sqlite PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(boost_sqlite PUBLIC Boost::system Boost::json SQLite::SQLite3)
target_compile_definitions(boost_sqlite PUBLIC BOOST_SQLITE_SEPARATE_COMPILATION=1 BOOST_FILESYSTEM_NO_DEPRECATED=1)
add_library(Boost::sqlite ALIAS boost_sqlite)

add_library(boost_sqlite_ext src/src_ext.cpp)
target_include_directories(boost_sqlite_ext PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(boost_sqlite_ext PUBLIC Boost::system Boost::json SQLite::SQLite3)
target_compile_definitions(boost_sqlite_ext PUBLIC BOOST_SQLITE_SEPARATE_COMPILATION=1 BOOST_FILESYSTEM_NO_DEPRECATED=1 BOOST_SQLITE_COMPILE_EXTENSION=1)
set_property(TARGET boost_sqlite_ext PROPERTY POSITION_INDEPENDENT_CODE ON)
add_library(Boost::sqlite_ext ALIAS boost_sqlite_ext)

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

if(BOOST_SQLITE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()


if(BOOST_SQLITE_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()
