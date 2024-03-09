//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/sqlite/resultset.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE


bool resultset::read_next(
              system::error_code & ec,
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
    else if (cc != SQLITE_ROW)
    {
        BOOST_SQLITE_ASSIGN_EC(ec, cc);
        ei.set_message(sqlite3_errmsg(sqlite3_db_handle(impl_.get())));
    }
    return !done_;
}

bool resultset::read_next()
{
    system::error_code ec;
    error_info ei;
    auto tmp = read_next(ec, ei);
    if (ec)
        throw_exception(system::system_error(ec, ei.message()));
    return tmp;
}

resultset::iterator resultset::iterator::operator++()
{
  if (sentinel_)
    return *this;

  auto cc = sqlite3_step(row_.stm_);
  if (cc == SQLITE_DONE)
    sentinel_ = true;
  else if (cc != SQLITE_ROW)
  {
    system::error_code ec;
    BOOST_SQLITE_ASSIGN_EC(ec, cc);
    throw_exception(system::system_error(ec, sqlite3_errmsg(sqlite3_db_handle(row_.stm_))));
  }
  return *this;
}

BOOST_SQLITE_END_NAMESPACE

