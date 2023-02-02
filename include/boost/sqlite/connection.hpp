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

struct connection
{
    using native_handle_type = sqlite3*;
    native_handle_type native_handle() { return impl_.get(); }
    native_handle_type release() &&    { return impl_.release(); }

    connection() = default;
    explicit connection(native_handle_type native_handle) : impl_(native_handle) {}
    connection(connection && ) = default;
    connection& operator=(connection && ) = default;

    connection(const char * filename) { connect(filename); }


    BOOST_SQLITE_DECL void connect(const char * filename);
    BOOST_SQLITE_DECL void connect(const char * filename, system::error_code & ec);
    BOOST_SQLITE_DECL void close();
    BOOST_SQLITE_DECL void close(error_code & ec, error_info & ei);
    bool valid() const {return impl_ != nullptr;}

    BOOST_SQLITE_DECL resultset query(
            core::string_view q,
            error_code & ec,
            error_info & ei);

    BOOST_SQLITE_DECL resultset query(core::string_view q);
    BOOST_SQLITE_DECL statement prepare_statement(
            core::string_view q,
            error_code & ec,
            error_info & ei);

    BOOST_SQLITE_DECL statement prepare_statement(core::string_view q);
    std::size_t changes() const
    {
        return sqlite3_changes(impl_.get());
    }

    std::size_t total_changes() const
    {
        return sqlite3_total_changes(impl_.get());
    }

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
