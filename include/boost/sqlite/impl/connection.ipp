//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_IMPL_CONNECTION_IPP
#define BOOST_SQLITE_IMPL_CONNECTION_IPP

#include <boost/sqlite/connection.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE



void connection::connect(const char * filename, int flags)
{
    system::error_code ec;
    connect(filename, flags, ec);
    if (ec)
        throw_exception(system::system_error(ec, "connect"));
}

void connection::connect(const char * filename, int flags, system::error_code & ec)
{
    sqlite3 * res;
    auto r = sqlite3_open_v2(filename, &res, flags,
                             nullptr);
    if (r != SQLITE_OK)
        BOOST_SQLITE_ASSIGN_EC(ec, r)
    else
    {
      impl_.reset(res);
      impl_.get_deleter().owned_ = true;
    }
    sqlite3_extended_result_codes(impl_.get(), true);
}

void connection::close()
{
    system::error_code ec;
    error_info ei;
    close(ec, ei);
    if (ec)
        throw_exception(system::system_error(ec, ei.message()));
}

void connection::close(system::error_code & ec,
                       error_info & ei)
{
    if (impl_)
    {
        auto tmp = impl_.release();
        auto cc = sqlite3_close(tmp);
        if (SQLITE_OK != cc)
        {
            impl_.reset(tmp);
            BOOST_SQLITE_ASSIGN_EC(ec, cc)
            ei.set_message(sqlite3_errmsg(impl_.get()));
        }
    }
}


resultset connection::query(
        core::string_view q,
        system::error_code & ec,
        error_info & ei)
{
    resultset res;
    sqlite3_stmt * ss;
    const auto cc = sqlite3_prepare_v2(impl_.get(),
                       q.data(), static_cast<int>(q.size()),
                       &ss, nullptr);

    if (cc != SQLITE_OK)
    {
        BOOST_SQLITE_ASSIGN_EC(ec, cc)
        ei.set_message(sqlite3_errmsg(impl_.get()));
    }
    else
    {
      res.impl_.reset(ss);
      if (!ec)
        res.read_next(ec, ei);
    }
    return res;
}

resultset connection::query(core::string_view q)
{
    system::error_code ec;
    error_info ei;
    auto tmp = query(q, ec, ei);
    if (ec)
        throw_exception(system::system_error(ec, ei.message()));
    return tmp;
}

statement connection::prepare(
        core::string_view q,
        system::error_code & ec,
        error_info & ei)
{
    statement res;
    sqlite3_stmt * ss;
    const auto cc = sqlite3_prepare_v2(impl_.get(),
                                       q.data(), static_cast<int>(q.size()),
                                       &ss, nullptr);

    if (cc != SQLITE_OK)
    {
        BOOST_SQLITE_ASSIGN_EC(ec, cc)
        ei.set_message(sqlite3_errmsg(impl_.get()));
    }
    else
        res.impl_.reset(ss);
    return res;
}

statement connection::prepare(core::string_view q)
{
    system::error_code ec;
    error_info ei;
    auto tmp = prepare(q, ec, ei);
    if (ec)
        throw_exception(system::system_error(ec, ei.message()));
    return tmp;
}

void connection::execute(
    const char * q,
    system::error_code & ec,
    error_info & ei)
{
    char * msg = nullptr;

    auto res = sqlite3_exec(impl_.get(), q, nullptr, nullptr, &msg);
    if (res != SQLITE_OK)
    {
        BOOST_SQLITE_ASSIGN_EC(ec, res);
        if (msg != nullptr)
            ei.set_message(msg);
    }
}

void connection::execute(const char * q)
{
    system::error_code ec;
    error_info ei;
    execute(q, ec, ei);
    if (ec)
      throw_exception(system::system_error(ec, ei.message()));
}

BOOST_SQLITE_END_NAMESPACE


#endif //BOOST_SQLITE_IMPL_CONNECTION_IPP
