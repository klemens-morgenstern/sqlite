// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_CONNECTION_HPP
#define BOOST_SQLITE_CONNECTION_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/error.hpp>
#include <boost/sqlite/statement.hpp>
#include <memory>
#include <boost/system/system_error.hpp>
#include <boost/sqlite/connection_ref.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE

constexpr static cstring_ref in_memory = ":memory:";

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
    /// Returns the handle
    handle_type handle() const { return impl_.get(); }
    /// Release the owned handle.
    handle_type release() &&    { return impl_.release(); }

    ///Default constructor
    connection() = default;
    /// Construct the connection from a handle.
    explicit connection(handle_type handle) : impl_(handle) {}
    /// Move constructor.
    connection(connection&& ) = default;
    /// Move assign operator.
    connection& operator=(connection&& ) = default;

    /// Construct a connection and connect it to `filename`. `flags` is set by `SQLITE_OPEN_*` flags. @see https://www.sqlite.org/c3ref/c_open_autoproxy.html
    connection(cstring_ref filename,
               int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE) { connect(filename, flags); }

#if defined(BOOST_WINDOWS_API)
    template<typename Path,
             typename = std::enable_if_t<
                 std::is_same<typename Path::string_type, std::wstring>::value &&
                 std::is_constructible<cstring_ref, decltype(std::declval<Path>().string())>::value
             >>
    explicit connection(const Path & pth) : connection(pth.string()) {}
#endif
    ///@{
    /// Connect the database to `filename`.  `flags` is set by `SQLITE_OPEN_*` flags.
    BOOST_SQLITE_DECL void connect(cstring_ref filename, int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    BOOST_SQLITE_DECL void connect(cstring_ref filename, int flags, system::error_code & ec);
    ///@}

#if defined(BOOST_WINDOWS_API)
    template<typename Path,
             typename = std::enable_if_t<
                 std::is_same<typename Path::string_type, std::wstring>::value &&
                 std::is_constructible<cstring_ref, decltype(std::declval<Path>().string())>::value
             >>
    void connect(const Path & pth, int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE)
    {
        connect(pth.string(), flags);
    }


    template<typename Path,
             typename = std::enable_if_t<
                 std::is_same<typename Path::string_type, std::wstring>::value &&
                 std::is_constructible<cstring_ref, decltype(std::declval<Path>().string())>::value
             >>
    void connect(const Path & pth, int flags, system::error_code & ec)
    {
        connect(pth.string(), flags, ec);
    }
#endif

    ///@{
    /// Close the database connection.
    BOOST_SQLITE_DECL void close();
    BOOST_SQLITE_DECL void close(system::error_code & ec, error_info & ei);
    ///@}

    /// Check if the database holds a valid handle.
    bool valid() const {return impl_ != nullptr;}

    BOOST_SQLITE_DECL void execute(
        core::string_view q,
        system::error_code &ec,
        error_info & ei);

    BOOST_SQLITE_DECL void execute(core::string_view q);

    ///@{
    /// Preparse a query with or without bound parameters. Can only contain a single statement.
    BOOST_SQLITE_DECL statement prepare(
            core::string_view q,
            system::error_code & ec,
            error_info & ei);

    BOOST_SQLITE_DECL statement prepare(core::string_view q);

    BOOST_SQLITE_DECL
    statement_list prepare_many(
        core::string_view q);

    BOOST_SQLITE_DECL
    statement_list prepare_many(
        core::string_view q,
        system::error_code & ec,
        error_info & ei);

    ///@}

    /// Check if the database has the table
    bool has_table(
        cstring_ref table,
        cstring_ref db_name = "main") const
    {
      return sqlite3_table_column_metadata(impl_.get(), db_name.c_str(), table.c_str(),
                                           nullptr, nullptr, nullptr, nullptr, nullptr, nullptr)
             == SQLITE_OK;
    }

    /// Check if the database has the table
    bool has_column(
        cstring_ref table,
        cstring_ref column,
        cstring_ref db_name = "main") const
    {
      return sqlite3_table_column_metadata(impl_.get(), db_name.c_str(), table.c_str(), column.c_str(),
                                           nullptr, nullptr, nullptr, nullptr, nullptr)
             == SQLITE_OK;
    }
 private:
    struct deleter_
    {
        deleter_() = default;
        void operator()(sqlite3  *impl)
        {
              sqlite3_close_v2(impl);
        }
    };

    std::unique_ptr<sqlite3, deleter_> impl_{nullptr, deleter_{}};
};

BOOST_SQLITE_END_NAMESPACE


#endif //BOOST_SQLITE_CONNECTION_HPP
