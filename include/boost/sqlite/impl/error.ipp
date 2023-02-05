//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_IMPL_ERROR_IPP
#define BOOST_SQLITE_IMPL_ERROR_IPP

#include <boost/sqlite/error.hpp>
#include <boost/core/detail/string_view.hpp>
#include <algorithm>

namespace boost
{
namespace sqlite
{

struct sqlite_category_t final : system::error_category
{
  sqlite_category_t() : system::error_category(0x7d4c7b49d8a3edull) {}

  std::string message( int ev ) const override
  {
    return message(ev, nullptr, 0u);
  }
  char const * message( int ev, char * buffer, std::size_t len ) const noexcept override
  {

    switch (ev)
    {
    case SQLITE_OK:          return "Successful result";
    case SQLITE_ERROR:       return "Generic error";
    case SQLITE_INTERNAL:    return "Internal logic error in SQLite";
    case SQLITE_PERM:        return "Access permission denied";
    case SQLITE_ABORT:       return "Callback routine requested an abort";
    case SQLITE_BUSY:        return "The database file is locked";
    case SQLITE_LOCKED:      return "A table in the database is locked";
    case SQLITE_NOMEM:       return "A malloc() failed";
    case SQLITE_READONLY:    return "Attempt to write a readonly database";
    case SQLITE_INTERRUPT:   return "Operation terminated by sqlite3_interrupt(";
    case SQLITE_IOERR:       return "Some kind of disk I/O error occurred";
    case SQLITE_CORRUPT:     return "The database disk image is malformed";
    case SQLITE_NOTFOUND:    return "Unknown opcode in sqlite3_file_control()";
    case SQLITE_FULL:        return "Insertion failed because database is full";
    case SQLITE_CANTOPEN:    return "Unable to open the database file";
    case SQLITE_PROTOCOL:    return "Database lock protocol error";
    case SQLITE_EMPTY:       return "Internal use only";
    case SQLITE_SCHEMA:      return "The database schema changed";
    case SQLITE_TOOBIG:      return "String or BLOB exceeds size limit";
    case SQLITE_CONSTRAINT:  return "Abort due to constraint violation";
    case SQLITE_MISMATCH:    return "Data type mismatch";
    case SQLITE_MISUSE:      return "Library used incorrectly";
    case SQLITE_NOLFS:       return "Uses OS features not supported on host";
    case SQLITE_AUTH:        return "Authorization denied";
    case SQLITE_FORMAT:      return "Not used";
    case SQLITE_RANGE:       return "2nd parameter to sqlite3_bind out of range";
    case SQLITE_NOTADB:      return "File opened that is not a database file";
    case SQLITE_NOTICE:      return "Notifications from sqlite3_log()";
    case SQLITE_WARNING:     return "Warnings from sqlite3_log()";
    case SQLITE_ROW:         return "sqlite3_step() has another row ready";
    case SQLITE_DONE:        return "sqlite3_step() has finished executing";
    default: return "unknown error";
    }
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

BOOST_SQLITE_DECL
error_code
make_error_code(error e)
{
  return error_code(static_cast<int>(e), sqlite_category());
}

} // sqlite
} // boost


#endif // BOOST_SQLITE_IMPL_ERROR_IPP
