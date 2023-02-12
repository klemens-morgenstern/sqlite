//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/sqlite/allocator.hpp>
#include "doctest.h"

using namespace boost;

TEST_CASE("allocator")
{

  sqlite::allocator<int> alloc;

  auto p = alloc.allocate(32);
  alloc.deallocate(p, 32);
}