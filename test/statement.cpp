//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <sqlite3.h>
#include "test.hpp"
#include <boost/sqlite/connection.hpp>

using namespace boost;


TEST_CASE("connection")
{
  sqlite::connection conn;
  conn.connect(":memory:");

  std::unique_ptr<int> data{new int(42)};

  auto ip = data.get();
  auto q = conn.prepare("select $1;").execute(std::make_tuple(std::move(data)));
  sqlite::row r;
  q.read_one(r);
  CHECK(r.size() == 1u);

  auto v = r.at(0).get_value();
  CHECK(v.type() == sqlite::value_type::null);
  CHECK(v.get_pointer<int>() != nullptr);
  CHECK(v.get_pointer<int>() == ip);
  CHECK(v.get_pointer<double>() == nullptr);

}


TEST_CASE("decltype")
{
  sqlite::connection conn;
  conn.connect(":memory:");
  conn.execute(
#include "test-db.sql"
  );
  auto q = conn.prepare("select* from author;");

  CHECK(q.declared_type(0) == "INTEGER");
  CHECK(q.declared_type(1) == "TEXT");
  CHECK(q.declared_type(2) == "TEXT");

}