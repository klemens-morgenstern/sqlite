//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/sqlite/allocator.hpp>
#include <boost/test/unit_test.hpp>

using namespace boost;

BOOST_AUTO_TEST_CASE(allocator)
{

  sqlite::allocator<int> alloc;

  auto p = alloc.allocate(32);
  BOOST_CHECK(p != nullptr);
  alloc.deallocate(p, 32);

  BOOST_CHECK_THROW(boost::ignore_unused(alloc.allocate((std::numeric_limits<std::size_t>::max)())), std::bad_alloc);
}
