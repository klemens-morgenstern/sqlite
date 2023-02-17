//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_IMPL_RESULTSET_IPP
#define BOOST_SQLITE_IMPL_RESULTSET_IPP

BOOST_SQLITE_BEGIN_NAMESPACE


bool resultset::read_one(row& r,
              error_code & ec,
              error_info & ei) // could also return row* instead!
{
    if (done_)
        return false;
    auto cc = sqlite3_step(impl_.get());
    if (cc == SQLITE_DONE)
    {
        done_ = true;
        return false;
    }
    else if (cc == SQLITE_ROW)
        r.stm_ = impl_.get();
    else
    {
        BOOST_SQLITE_ASSIGN_EC(ec, cc);
        ei.set_message(sqlite3_errmsg(sqlite3_db_handle(impl_.get())));
    }
    return !done_;
}

bool resultset::read_one(row & r)
{
    system::error_code ec;
    error_info ei;
    auto tmp = read_one(r, ec, ei);
    if (ec)
        throw_exception(system::system_error(ec, ei.message()));
    return tmp;
}

system::result<row> resultset::read_one()
{
    system::error_code ec;
    error_info ei;
    row r;
    read_one(r, ec, ei);
    if (ec)
        return ec;
    else
        return r;
}

BOOST_SQLITE_END_NAMESPACE

#endif //BOOST_SQLITE_IMPL_RESULTSET_IPP
