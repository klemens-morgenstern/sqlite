//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_IMPL_ERROR_IPP
#define BOOST_SQLITE_IMPL_ERROR_IPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/error.hpp>
#include <boost/core/detail/string_view.hpp>
#include <algorithm>

BOOST_SQLITE_BEGIN_NAMESPACE

struct sqlite_category_t final : system::error_category
{
#if defined(BOOST_SQLITE_COMPILE_EXTENSION)
  sqlite_category_t() : system::error_category(0x7d4c7b49d8a3edull) {}
#else
  sqlite_category_t() : system::error_category(0x7d4c7b49d8a3fdull) {}
#endif


  std::string message( int ev ) const override
  {
    return sqlite3_errstr(ev);
  }
  char const * message( int ev, char * buffer, std::size_t len ) const noexcept override
  {
    std::snprintf( buffer, len, "%s", sqlite3_errstr( ev ) );
    return buffer;
  }

  const char * name() const BOOST_NOEXCEPT override
  {
    return "sqlite3";
  }
};

BOOST_SQLITE_DECL
error_category & sqlite_category()
{
  static sqlite_category_t cat;
  return cat;
}

BOOST_SQLITE_END_NAMESPACE

#endif // BOOST_SQLITE_IMPL_ERROR_IPP
