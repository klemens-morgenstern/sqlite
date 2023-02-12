//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


#include <boost/sqlite/mutex.hpp>

#include "doctest.h"

#include <mutex>

using namespace boost;

TEST_CASE("mutex")
{
  sqlite::mutex mtx;
  sqlite::recursive_mutex rmtx;
  std::scoped_lock<sqlite::mutex, sqlite::recursive_mutex> lk{mtx, rmtx};
  std::lock_guard<sqlite::recursive_mutex> l{rmtx};
}