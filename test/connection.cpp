// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/sqlite/connection.hpp>
#include "test.hpp"

using namespace boost;

TEST_CASE("connection")
{
  sqlite::connection conn;
  conn.connect(":memory:");
  conn.execute(
#include "test-db.sql"
  );

  conn.close();
}