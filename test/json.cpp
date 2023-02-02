// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/sqlite/json.hpp>
#include <boost/sqlite/connection.hpp>
#include <boost/json.hpp>
#include "test.hpp"

using namespace boost;

TEST_CASE("json")
{
  sqlite::connection conn(":memory:");
  conn.execute(
#include "test-db.sql"
  );

  auto q = conn.prepare("select 'foo', json_array($1, '2', null)").execute(std::make_tuple(1));

  sqlite::row r;
  q.read_one(r);
  CHECK(!sqlite::is_json(r[0]));
  CHECK( sqlite::is_json(r[1]));

  CHECK(sqlite::as_json(r[1]) == json::array{1, "2", nullptr});
  CHECK(json::value_from(r[1]) == json::array{1, "2", nullptr});

  // language=sqlite
  q = conn.query(R"(select first_name, "name" from library inner join author a on a.id = library.author order by library.name asc)");

  auto js = json::value_from(std::move(q));

  json::array aa {
      {
          {"first_name", "vinnie"},
          {"name",       "beast"}
      },
      {
          {"first_name", "peter"},
          {"name",       "mp11"}
      },
      {
          {"first_name",  "ruben"},
          {"name",        "mysql"}
      },
      {
          {"first_name","peter"},
          {"name",      "variant2"}
      },
  };

  CHECK(aa == aa);
};
