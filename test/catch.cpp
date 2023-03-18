// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/sqlite/detail/catch.hpp>
#include "doctest.h"

using namespace boost;

TEST_CASE("prefix")
{
  system::system_error se(SQLITE_TOOBIG, sqlite::sqlite_category());
  CHECK(sqlite::detail::get_message(se).empty());

  se = system::system_error(SQLITE_TOOBIG, sqlite::sqlite_category(), "foobar");

  CHECK(sqlite::detail::get_message(se) == "foobar");
}