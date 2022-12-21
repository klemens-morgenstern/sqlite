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
    native_handle_type native_handle()
    {
        return impl_.get();
    }

    connection() = default;
    explicit connection(native_handle_type native_handle) : impl_(native_handle) {}
    connection(connection && ) = default;
    connection& operator=(connection && ) = default;

    connection(const char * filename) { connect(filename); }


    void connect(const char * filename)
    {
        system::error_code ec;
        connect(filename, ec);
        if (ec)
            throw_exception(system::system_error(ec, "connect"));
    }

    void connect(const char * filename, system::error_code & ec)
    {
        sqlite3 * res;
        auto r = sqlite3_open(filename, &res);
        if (r != SQLITE_OK)
            BOOST_SQLITE_ASSIGN_EC(ec, r)
        else
            impl_.reset(res);
    }

    void close()
    {
        system::error_code ec;
        error_info ei;
        close(ec, ei);
        if (ec)
            throw_exception(system::system_error(ec, ei.message()));
    }

    void close(error_code & ec,
               error_info & ei)
    {
        if (impl_)
        {
            auto tmp = impl_.release();
            auto cc = sqlite3_close(tmp);
            if (SQLITE_OK != cc)
            {
                impl_.reset(tmp);
                BOOST_SQLITE_ASSIGN_EC(ec, cc);
                ei.set_message(sqlite3_errmsg(impl_.get()));
            }
        }
    }
    bool valid() const {return impl_ != nullptr;}

    resultset query(
            core::string_view q,
            error_code & ec,
            error_info & ei)
    {
        resultset res;
        sqlite3_stmt * ss;
        const auto cc = sqlite3_prepare_v2(impl_.get(),
                           q.data(), q.size(),
                           &ss, nullptr);

        if (cc != SQLITE_OK)
        {
            BOOST_SQLITE_ASSIGN_EC(ec, cc);
            ei.set_message(sqlite3_errmsg(impl_.get()));
        }
        else
            res.impl_.reset(ss);
        return res;
    }

    resultset query(core::string_view q)
    {
        system::error_code ec;
        error_info ei;
        auto tmp = query(q, ec, ei);
        if (ec)
            throw_exception(system::system_error(ec, ei.message()));
        return tmp;
    }

    statement prepare_statement(
            core::string_view q,
            error_code & ec,
            error_info & ei)
    {
        statement res;
        sqlite3_stmt * ss;
        const auto cc = sqlite3_prepare_v2(impl_.get(),
                                           q.data(), q.size(),
                                           &ss, nullptr);

        if (cc != SQLITE_OK)
        {
            BOOST_SQLITE_ASSIGN_EC(ec, cc);
            ei.set_message(sqlite3_errmsg(impl_.get()));
        }
        else
            res.impl_.reset(ss);
        return res;
    }

    statement prepare_statement(core::string_view q)
    {
        system::error_code ec;
        error_info ei;
        auto tmp = prepare_statement(q, ec, ei);
        if (ec)
            throw_exception(system::system_error(ec, ei.message()));
        return tmp;
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
