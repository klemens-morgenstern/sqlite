// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_CONNECTION_HPP
#define BOOST_SQLITE_CONNECTION_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/error.hpp>
#include <boost/sqlite/resultset.hpp>
#include <boost/sqlite/statement.hpp>
#include <sqlite3.h>
#include <memory>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

namespace boost {
namespace sqlite {

/** @brief main object for a connection to a database.
  @ingroup reference

  @par Example
  @code{.cpp}
    sqlite::connection conn;
    conn.connect("./my-database.db");
    conn.prepare("insert into log (text) values ($1)").execute(std::make_tuple("booting up"));
  @endcode

 */
struct connection
{
    /// The handle of the connection
    using handle_type = sqlite3*;
    /// Get the handle
    handle_type handle() { return impl_.get(); }
    /// Release the owned handle.
    handle_type release() &&    { return impl_.release(); }

    ///Default constructor
    connection() = default;
    /// Construct the connection from a handle. This will take ownership.
    explicit connection(handle_type handle) : impl_(handle) {}
    /// Move constructor.
    connection(connection && ) = default;
    /// Move assign operator.
    connection& operator=(connection && ) = default;

    /// Construct a connection and connect it to `filename`.. `flags` is set by `SQLITE_OPEN_*` flags.
    connection(const char * filename,
               int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE) { connect(filename, flags); }


    ///@{
    /// Connect the database to `filename`.  `flags` is set by `SQLITE_OPEN_*` flags.
    BOOST_SQLITE_DECL void connect(const char * filename, int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    BOOST_SQLITE_DECL void connect(const char * filename, int flags, system::error_code & ec);
    ///@}

    ///@{
    /// Close the database connection.
    BOOST_SQLITE_DECL void close();
    BOOST_SQLITE_DECL void close(error_code & ec, error_info & ei);
    ///@}

    /// Check if the database holds a valid handle.
    bool valid() const {return impl_ != nullptr;}


    ///@{
    /// Perform a query without parameters. Can only execute a single statement.
    BOOST_SQLITE_DECL resultset query(
            core::string_view q,
            error_code & ec,
            error_info & ei);

    BOOST_SQLITE_DECL resultset query(core::string_view q);
    ///@}

    ///@{
    /// Perform a query without parametert, It execute a multiple statement.
    BOOST_SQLITE_DECL void execute(
      const char * q,
        error_code & ec,
        error_info & ei);

    BOOST_SQLITE_DECL void execute(const char * q);
    BOOST_SQLITE_DECL void execute(
        const std::string & q,
        error_code & ec,
        error_info & ei)
    {
        execute(q.c_str(), ec, ei);
    }
    ///@}

    void execute(const std::string & q) { execute(q.c_str());}

    ///@{
    /// Perform a query with parameters. Can only execute a single statement.
    BOOST_SQLITE_DECL statement prepare(
            core::string_view q,
            error_code & ec,
            error_info & ei);

    BOOST_SQLITE_DECL statement prepare(core::string_view q);
    ///@}

    /// The changes applied to the database.
    std::size_t changes() const
    {
        return sqlite3_changes(impl_.get());
    }
    /// The total changes applied to the database.
    std::size_t total_changes() const
    {
        return sqlite3_total_changes(impl_.get());
    }
    /// Get the filename associated with the database.
    core::string_view filename(const std::string & db_name = "main")
    {
       return sqlite3_db_filename(impl_.get(), db_name.c_str());
    }
 private:
    struct deleter_
    {
        void operator()(sqlite3  *impl)
        {
            sqlite3_close_v2(impl);
        }
    };

    std::unique_ptr<sqlite3, deleter_> impl_;
};

}
}

#endif //BOOST_SQLITE_CONNECTION_HPP
