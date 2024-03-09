//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


#include <boost/sqlite/mutex.hpp>

#include <boost/test/unit_test.hpp>
#include <mutex>

using namespace boost;

BOOST_AUTO_TEST_CASE(mutex)
{
  sqlite::mutex mtx;
  sqlite::recursive_mutex rmtx;
  BOOST_CHECK(mtx.try_lock());
  BOOST_CHECK(!mtx.try_lock());
  mtx.unlock();

  std::lock_guard<sqlite::mutex> l1{mtx};
  std::lock_guard<sqlite::recursive_mutex> l2{rmtx};
  std::lock_guard<sqlite::recursive_mutex> l{rmtx};

  BOOST_CHECK(rmtx.try_lock());
  BOOST_CHECK(rmtx.try_lock());
  BOOST_CHECK(rmtx.try_lock());
}