// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_CONNECTION_REF_HPP
#define BOOST_SQLITE_CONNECTION_REF_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/error.hpp>
#include <boost/sqlite/statement.hpp>
#include <memory>
#include <boost/system/system_error.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE

struct connection;

/** @brief main object for a connection_ref to a database.
  @ingroup reference

  @par Example
  @code{.cpp}
    sqlite::connection_ref conn;
    conn.connect("./my-database.db");
    conn.prepare("insert into log (text) values ($1)").execute(std::make_tuple("booting up"));
  @endcode

 */
struct connection_ref
{
    /// The handle of the connection_ref
    using handle_type = sqlite3*;
    /// Returns the handle
    handle_type handle() const { return impl_; }

    ///Default constructor
    connection_ref() = default;
    /// Construct the connection_ref from a handle.
    explicit connection_ref(handle_type handle) : impl_(handle) {}
    /// Construct the connection_ref from a handle.
    BOOST_SQLITE_DECL connection_ref(connection & conn);
    /// Move constructor.
    connection_ref(const connection_ref & ) = default;
    /// Move assign operator.
    connection_ref& operator=(const connection_ref & ) = default;

    /// Check if the database holds a valid handle.
    bool valid() const {return impl_ != nullptr;}

    /// Preparse a query with or without bound parameters. Can only contain a single statement.
    BOOST_SQLITE_DECL statement prepare(
            core::string_view q,
            system::error_code & ec,
            error_info & ei);

    BOOST_SQLITE_DECL statement prepare(core::string_view q);

    ///@}

    BOOST_SQLITE_DECL
    statement_list prepare_many(
        core::string_view q);

    BOOST_SQLITE_DECL
    statement_list prepare_many(
        core::string_view q,
        system::error_code & ec,
        error_info & ei);

    /// Execute
    BOOST_SQLITE_DECL void execute(
        std::string_view q,
        system::error_code &ec,
        error_info & ei);

    BOOST_SQLITE_DECL void execute(std::string_view q);

    /// Check if the database has the table
    bool has_table(
        cstring_ref table,
        cstring_ref db_name = "main") const
    {
      return sqlite3_table_column_metadata(impl_, db_name.c_str(), table.c_str(),
                                           nullptr, nullptr, nullptr, nullptr, nullptr, nullptr)
             == SQLITE_OK;
    }

    /// Check if the database has the table
    bool has_column(
        cstring_ref table,
        cstring_ref column,
        cstring_ref db_name = "main") const
    {
      return sqlite3_table_column_metadata(impl_, db_name.c_str(), table.c_str(), column.c_str(),
                                           nullptr, nullptr, nullptr, nullptr, nullptr)
             == SQLITE_OK;
    }
 private:
    sqlite3* impl_ = nullptr;
};

BOOST_SQLITE_END_NAMESPACE


#endif //BOOST_SQLITE_CONNECTION_REF_HPP
