//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_EXTENSION_HPP
#define BOOST_SQLITE_EXTENSION_HPP

#define BOOST_SQLITE_COMPILE_EXTENSION 1
#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/connection.hpp>
#include <boost/sqlite/detail/catch.hpp>
#include <boost/config.hpp>


/** @brief Declare a sqlite module.
 @ingroup reference

 @param Name The name of the module
 @param Conn The parameter name of the connection

 This macro can be used to create an sqlite extension.

 @note When defining BOOST_SQLITE_COMPILE_EXTENSION (was is done in extension.hpp)
        sqlite will use an inline namespace to avoid symbol clashes.

 @par Examples

 @code{.cpp}

 BOOST_SQLITE_EXTENSION(extension, conn)
 {
    create_scalar_function(
        conn, "assert",
        [](boost::sqlite::context<>, boost::span<boost::sqlite::value, 1u> sp)
        {
            if (sp.front().get_int() == 0)
              throw std::logic_error("assertion failed");
        });
 }

 @endcode{.cpp}

 */
#define BOOST_SQLITE_EXTENSION(Name, Conn)                        \
void sqlite_##Name##_impl (boost::sqlite::connection Conn);       \
extern "C" BOOST_SYMBOL_EXPORT                                    \
int sqlite3_##Name##_init(                                        \
    sqlite3 *db,                                                  \
    char **pzErrMsg,                                              \
    const sqlite3_api_routines *pApi)                             \
{                                                                 \
  using boost::sqlite::sqlite3_api;                               \
  BOOST_SQLITE_TRY                                                \
  {                                                               \
      using boost::sqlite::sqlite3_api;                           \
      SQLITE_EXTENSION_INIT2(pApi);                               \
      sqlite_##Name##_impl(boost::sqlite::connection{db, false}); \
  }                                                               \
  BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(*pzErrMsg)             \
  return SQLITE_OK;                                               \
}                                                                 \
void sqlite_##Name##_impl(boost::sqlite::connection Conn)

#endif //BOOST_SQLITE_EXTENSION_HPP
