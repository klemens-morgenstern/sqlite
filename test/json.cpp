// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/sqlite/json.hpp>
#include <boost/sqlite/connection.hpp>
#include <boost/sqlite/function.hpp>
#include <boost/json.hpp>
#include "test.hpp"

using namespace boost;


TEST_SUITE_BEGIN("json");

TEST_CASE("to_value")
{
  sqlite::connection conn(":memory:");
  conn.execute(
#include "test-db.sql"
  );

  auto q = conn.prepare("select 'foo', json_array($1, '2', null)").execute(std::make_tuple(1));

  sqlite::row r = q.current();

  CHECK(!sqlite::is_json(r[0]));
  CHECK( sqlite::is_json(r[1]));
  CHECK(sqlite::as_json(r[1]) == json::array{1, "2", nullptr});
  CHECK(json::value_from(r[1]) == json::array{1, "2", nullptr});
  CHECK(!q.read_next());
  CHECK(!q.read_next());

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

TEST_CASE("blob")
{
  sqlite::connection conn(":memory:");
  CHECK_THROWS(json::value_from(conn.prepare("select $1;").execute({sqlite::zero_blob(1024)})));
  CHECK(nullptr == json::value_from(conn.query("select null;").current().at(0)));
  CHECK(1234 == json::value_from(conn.query("select 1234;").   current().at(0)));
  CHECK(12.4 == json::value_from(conn.query("select 12.4;").   current().at(0)));
}

TEST_CASE("value")
{
  sqlite::connection conn(":memory:");
  CHECK_THROWS(json::value_from(conn.prepare("select $1;").execute({sqlite::zero_blob(1024)}).current().at(0)));
  CHECK(nullptr == json::value_from(conn.query("select null;").current().at(0).get_value()));
  CHECK(1234 == json::value_from(conn.query("select 1234;").   current().at(0).get_value()));
  CHECK(12.4 == json::value_from(conn.query("select 12.4;").   current().at(0).get_value()));
  CHECK("txt" == json::value_from(conn.query("select 'txt';"). current().at(0).get_value()));
}


TEST_CASE("subtype")
{
  sqlite::connection conn(":memory:");
  CHECK(!sqlite::is_json(conn.prepare("select $1;").execute({"foobar"}).current().at(0)));
  CHECK(sqlite::is_json(conn.prepare("select json_array($1);").execute({"foobar"}).current().at(0)));
}


TEST_CASE("function")
{
  sqlite::connection conn(":memory:");
  sqlite::create_scalar_function(conn, "my_json_parse",
                                 [](boost::sqlite::context<> , boost::span<boost::sqlite::value, 1u> s)
                                 {
                                      return json::parse(s[0].get_text());
                                 });

  CHECK(sqlite::is_json(conn.prepare("select my_json_parse($1);")
            .execute({R"({"foo" : 42, "bar" : "xyz"})"}).current().at(0)));
}


TEST_SUITE_END();