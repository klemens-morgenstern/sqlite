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


BOOST_AUTO_TEST_SUITE(json_);

BOOST_AUTO_TEST_CASE(to_value)
{
  sqlite::connection conn(":memory:");
  conn.execute(
#include "test-db.sql"
  );

  auto q = conn.prepare("select 'foo', json_array($1, '2', null)").execute(std::make_tuple(1));

  sqlite::row r = q.current();

  BOOST_CHECK(!sqlite::is_json(r[0]));
  BOOST_CHECK( sqlite::is_json(r[1]));
  BOOST_CHECK(sqlite::as_json(r[1])  == (json::array{1, "2", nullptr}));
  BOOST_CHECK(json::value_from(r[1]) == (json::array{1, "2", nullptr}));
  BOOST_CHECK(!q.read_next());
  BOOST_CHECK(!q.read_next());

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

  BOOST_CHECK(aa == aa);
};

BOOST_AUTO_TEST_CASE(blob)
{
  sqlite::connection conn(":memory:");
  BOOST_CHECK_THROW(json::value_from(conn.prepare("select $1;").execute({sqlite::zero_blob(1024)})), std::invalid_argument);
  BOOST_CHECK(nullptr == json::value_from(conn.query("select null;").current().at(0)));
  BOOST_CHECK(1234 == json::value_from(conn.query("select 1234;").   current().at(0)));
  BOOST_CHECK(12.4 == json::value_from(conn.query("select 12.4;").   current().at(0)));
}

BOOST_AUTO_TEST_CASE(value)
{
  sqlite::connection conn(":memory:");
  BOOST_CHECK_THROW(json::value_from(conn.prepare("select $1;").execute({sqlite::zero_blob(1024)}).current().at(0)), std::invalid_argument);
  BOOST_CHECK(nullptr == json::value_from(conn.query("select null;").current().at(0).get_value()));
  BOOST_CHECK(1234 == json::value_from(conn.query("select 1234;").   current().at(0).get_value()));
  BOOST_CHECK(12.4 == json::value_from(conn.query("select 12.4;").   current().at(0).get_value()));
  BOOST_CHECK("txt" == json::value_from(conn.query("select 'txt';"). current().at(0).get_value()));
}


BOOST_AUTO_TEST_CASE(subtype)
{
  sqlite::connection conn(":memory:");
  BOOST_CHECK(!sqlite::is_json(conn.prepare("select $1;").execute({"foobar"}).current().at(0)));
  BOOST_CHECK(sqlite::is_json(conn.prepare("select json_array($1);").execute({"foobar"}).current().at(0)));
}


BOOST_AUTO_TEST_CASE(function)
{
  sqlite::connection conn(":memory:");
  sqlite::create_scalar_function(conn, "my_json_parse",
                                 [](boost::sqlite::context<> , boost::span<boost::sqlite::value, 1u> s)
                                 {
                                      return json::parse(s[0].get_text());
                                 });

  BOOST_CHECK(sqlite::is_json(conn.prepare("select my_json_parse($1);")
            .execute({R"({"foo" : 42, "bar" : "xyz"})"}).current().at(0)));
}


BOOST_AUTO_TEST_SUITE_END();