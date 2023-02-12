// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/sqlite/field.hpp>
#include <boost/sqlite/connection.hpp>
#include "test.hpp"

using namespace boost;

TEST_CASE("field")
{
  sqlite::connection conn(":memory:");
  // language=sqlite
  conn.execute(R"(
create table type_tester(
  id integer primary key autoincrement,
  num real,
  nl null,
  txt text,
  blb blob);

  insert into type_tester values(42, 1.2, null, 'text', x'04050607');
)");

  auto res = conn.query("select * from type_tester");
  sqlite::row r;
  res.read_one(r);

  CHECK(r[0].type() == sqlite::value_type::integer);
  CHECK(r[0].get_int() == 42);

  CHECK(r[1].type() == sqlite::value_type::floating);
  CHECK(r[1].get_double() == 1.2);

  CHECK(r[2].type() == sqlite::value_type::null);
  CHECK(r[3].type() == sqlite::value_type::text);
  CHECK(r[3].get_text() == "text");

  CHECK(r[4].type() == sqlite::value_type::blob);

  sqlite::blob bl{4u};
  char raw_data[4] = {4,5,6,7};
  std::memcpy(bl.data(), raw_data, 4);
  CHECK(std::memcmp(bl.data(), r[4].get_blob().data(), 4u) == 0u);

  CHECK(conn.query("select null;").read_one()->at(0).get_text() == "");
  CHECK(conn.query("select null;").read_one()->at(0).get_value().get_text() == "");

}