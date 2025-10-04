//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


#include <boost/sqlite/connection_ref.hpp>
#include <boost/sqlite/connection.hpp>
#include <boost/sqlite/statement.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE


connection_ref::connection_ref(connection & conn) : impl_(conn.handle()) {}


statement connection_ref::prepare(
        core::string_view q,
        system::error_code & ec,
        error_info & ei)
{
    statement res;
    sqlite3_stmt * ss;
    const auto cc = sqlite3_prepare_v2(impl_,
                                       q.data(), static_cast<int>(q.size()),
                                       &ss, nullptr);

    if (cc != SQLITE_OK)
    {
        BOOST_SQLITE_ASSIGN_EC(ec, cc);
        ei.set_message(sqlite3_errmsg(impl_));
    }
    else
        res.impl_.reset(ss);
    return res;
}

statement connection_ref::prepare(core::string_view q)
{
    system::error_code ec;
    error_info ei;
    auto tmp = prepare(q, ec, ei);
    if (ec)
        throw_exception(system::system_error(ec, ei.message()));
    return tmp;
}


statement_list connection_ref::prepare_many(
        core::string_view q,
        system::error_code & ec,
        error_info & ei)
{
    statement res;
    sqlite3_stmt * ss;
    const char * tail = q.begin();
    const auto cc = sqlite3_prepare_v2(impl_,
                                       q.data(), static_cast<int>(q.size()),
                                       &ss, &tail);

    if (cc != SQLITE_OK)
    {
        BOOST_SQLITE_ASSIGN_EC(ec, cc);
        ei.set_message(sqlite3_errmsg(impl_));
    }
    else
        res.impl_.reset(ss);
    return {std::move(res), core::string_view(tail, q.end())};
}

statement_list connection_ref::prepare_many(core::string_view q)
{
    system::error_code ec;
    error_info ei;
    auto tmp = prepare_many(q, ec, ei);
    if (ec)
        throw_exception(system::system_error(ec, ei.message()));
    return tmp;
}

void connection_ref::execute(
        core::string_view q,
        system::error_code &ec,
        error_info & ei)
{
    auto sl = prepare_many(q, ec, ei);
    while (!sl.done() && !ec)
    {
        auto & s = sl.current();
        while (!s.done() && !ec)
            s.step(ec, ei);

        if (!ec)
            sl.prepare_next(ec, ei);
    }
}        

void connection_ref::execute(core::string_view q)
{
    system::error_code ec;
    error_info ei;
    execute(q, ec, ei);
    if (ec)
        throw_exception(system::system_error(ec, ei.message()));
}

BOOST_SQLITE_END_NAMESPACE


