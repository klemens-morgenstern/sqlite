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
#include <memory>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE

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
    /// Construct the connection from a handle.
    explicit connection(handle_type handle, bool take_ownership = true) : impl_(handle, take_ownership) {}
    /// Move constructor.
    connection(connection && ) = default;
    /// Move assign operator.
    connection& operator=(connection && ) = default;

    /// Construct a connection and connect it to `filename`.. `flags` is set by `SQLITE_OPEN_*` flags. @see https://www.sqlite.org/c3ref/c_open_autoproxy.html
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
    void execute(const std::string & q) { execute(q.c_str());}
    ///@}

    ///@{
    /// Perform a query with parameters. Can only execute a single statement.
    BOOST_SQLITE_DECL statement prepare(
            core::string_view q,
            error_code & ec,
            error_info & ei);

    BOOST_SQLITE_DECL statement prepare(core::string_view q);
    ///@}

    /// Check if the database has the table
    bool has_table(
        const char * table,
        const char * db_name = "main") const
    {
      return sqlite3_table_column_metadata(impl_.get(), db_name, table,
                                           nullptr, nullptr, nullptr, nullptr, nullptr, nullptr)
             == SQLITE_OK;
    }

    /// Check if the database has the table
    bool has_column(
        const char * table,
        const char * column,
        const char * db_name = "main") const
    {
      return sqlite3_table_column_metadata(impl_.get(), db_name, table, column,
                                           nullptr, nullptr, nullptr, nullptr, nullptr)
             == SQLITE_OK;
    }
 private:
    struct deleter_
    {
        deleter_(bool owned = true) : owned_(owned) {}
        bool owned_ = true;
        void operator()(sqlite3  *impl)
        {
            if (owned_)
              sqlite3_close_v2(impl);
        }
    };

    std::unique_ptr<sqlite3, deleter_> impl_{nullptr, deleter_{}};
};

BOOST_SQLITE_END_NAMESPACE

#if defined(BOOST_SQLITE_HEADER_ONLY)
#include <boost/sqlite/impl/connection.ipp>
#endif
#endif //BOOST_SQLITE_CONNECTION_HPP
