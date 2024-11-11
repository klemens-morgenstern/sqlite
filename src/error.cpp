//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


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

  bool failed( int ev ) const noexcept final
  {
    return ev != SQLITE_OK
        && ev != SQLITE_NOTICE
        && ev != SQLITE_WARNING
        && ev != SQLITE_ROW
        && ev != SQLITE_DONE;
  }

  std::string message( int ev ) const final
  {
    return sqlite3_errstr(ev);
  }
  char const * message( int ev, char * buffer, std::size_t len ) const noexcept final
  {
    std::snprintf( buffer, len, "%s", sqlite3_errstr( ev ) );
    return buffer;
  }

  const char * name() const BOOST_NOEXCEPT override
  {
    return "sqlite3";
  }

  system::error_condition default_error_condition( int ev ) const noexcept final
  {
    namespace errc = boost::system::errc;
    switch (ev & 0xFF)
    {
      case SQLITE_OK:         return {};
      case SQLITE_PERM:       return errc::permission_denied;
      case SQLITE_BUSY:       return errc::device_or_resource_busy;
      case SQLITE_NOMEM:      return errc::not_enough_memory;
      case SQLITE_INTERRUPT:  return errc::interrupted;
      case SQLITE_IOERR:      return errc::io_error;
    }

    return system::error_condition(ev, *this);
  }

};


system::error_category & sqlite_category()
{
  static sqlite_category_t cat;
  return cat;
}

void throw_exception_from_error( error const & e, boost::source_location const & loc )
{
  boost::throw_exception(
      system::system_error(e.code,
                           sqlite_category(),
                           e.info.message().c_str()), loc);
}


BOOST_SQLITE_END_NAMESPACE

