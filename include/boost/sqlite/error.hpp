//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_ERROR_HPP
#define BOOST_SQLITE_ERROR_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <sqlite3.h>


namespace boost {
namespace sqlite {

/// The type of error code used by the library
using error_code = boost::system::error_code;

/// The type of system error thrown by the library
using system_error = boost::system::system_error;

/// The type of error category used by the library
using error_category = boost::system::error_category;

/// Error codes returned from library operations
enum class error
{
    ok         =  SQLITE_OK,         /**< Successful result */
    error      =  SQLITE_ERROR,      /**< Generic error */
    internal   =  SQLITE_INTERNAL,   /**< Internal logic error in SQLite */
    perm       =  SQLITE_PERM,       /**< Access permission denied */
    abort      =  SQLITE_ABORT,      /**< Callback routine requested an abort */
    busy       =  SQLITE_BUSY,       /**< The database file is locked */
    locked     =  SQLITE_LOCKED,     /**< A table in the database is locked */
    nomem      =  SQLITE_NOMEM,      /**< A malloc() failed */
    readonly   =  SQLITE_READONLY,   /**< Attempt to write a readonly database */
    interrupt  =  SQLITE_INTERRUPT,  /**< Operation terminated by sqlite3_interrupt()*/
    ioerr      =  SQLITE_IOERR,      /**< Some kind of disk I/O error occurred */
    corrupt    =  SQLITE_CORRUPT,    /**< The database disk image is malformed */
    notfound   =  SQLITE_NOTFOUND,   /**< Unknown opcode in sqlite3_file_control() */
    full       =  SQLITE_FULL,       /**< Insertion failed because database is full */
    cantopen   =  SQLITE_CANTOPEN,   /**< Unable to open the database file */
    protocol   =  SQLITE_PROTOCOL,   /**< Database lock protocol error */
    empty      =  SQLITE_EMPTY,      /**< Internal use only */
    schema     =  SQLITE_SCHEMA,     /**< The database schema changed */
    toobig     =  SQLITE_TOOBIG,     /**< String or BLOB exceeds size limit */
    constraint =  SQLITE_CONSTRAINT, /**< Abort due to constraint violation */
    mismatch   =  SQLITE_MISMATCH,   /**< Data type mismatch */
    misuse     =  SQLITE_MISUSE,     /**< Library used incorrectly */
    nolfs      =  SQLITE_NOLFS,      /**< Uses OS features not supported on host */
    auth       =  SQLITE_AUTH,       /**< Authorization denied */
    format     =  SQLITE_FORMAT,     /**< Not used */
    range      =  SQLITE_RANGE,      /**< 2nd parameter to sqlite3_bind out of range */
    notadb     =  SQLITE_NOTADB,     /**< File opened that is not a database file */
    notice     =  SQLITE_NOTICE,     /**< Notifications from sqlite3_log() */
    warning    =  SQLITE_WARNING,    /**< Warnings from sqlite3_log() */
    row        =  SQLITE_ROW,        /**< sqlite3_step() has another row ready */
    done       =  SQLITE_DONE,       /**< sqlite3_step() has finished executing */
};

BOOST_SQLITE_DECL
error_category & sqlite_category();

BOOST_SQLITE_DECL
error_code
make_error_code(error e);

// copy-pastaed from anarthal/mysql --> alias ?

/**
 * \brief Additional information about error conditions
 * \details Contains an error message describing what happened. Not all error
 * conditions are able to generate this extended information - those that
 * can't have an empty error message.
 */
class error_info
{
    std::string msg_;

  public:
    /// Default constructor.
    error_info() = default;

    /// Initialization constructor.
    error_info(std::string&& err) noexcept : msg_(std::move(err)) {}

    /// Gets the error message.
    const std::string& message() const noexcept { return msg_; }

    /// Sets the error message.
    void set_message(std::string&& err) { msg_ = std::move(err); }

    /// Restores the object to its initial state.
    void clear() noexcept { msg_.clear(); }
};


} // requests

namespace system {


template<>
struct is_error_code_enum<::boost::sqlite::error>
{
  static bool const value = true;
};


} // system
} // boost

#if defined(BOOST_SQLITE_HEADER_ONLY)
#include <boost/sqlite/impl/error.ipp>
#endif

#endif // BOOST_SQLITE_ERROR_HPP
