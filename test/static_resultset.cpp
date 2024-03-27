//
// Copyright (c) 2024 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


#include <boost/sqlite/static_resultset.hpp>
#include <boost/sqlite/connection.hpp>

#include <boost/describe/class.hpp>

#include "test.hpp"

using namespace boost;

BOOST_AUTO_TEST_CASE(tuple)
{
        sqlite::connection conn;
    conn.connect(":memory:");
    conn.execute(
#include "test-db.sql"
    );

    using tup = std::tuple<sqlite_int64, sqlite::string_view, sqlite::string_view>;

    bool found = false;
    for (tup t : conn.query<tup>("select id, first_name, last_name from author where last_name = 'hodges';"))
    {
      BOOST_CHECK(std::get<1>(t) == "richard");
      found = true;
    }
    BOOST_CHECK(found);

    found = false;
    for (tup t : conn.prepare("select id, first_name, last_name from author where last_name = ?;")
                     .execute<tup>({"hodges"}))
    {
      BOOST_CHECK(std::get<1>(t) == "richard");
      found = true;
    }
    BOOST_CHECK(found);

    BOOST_CHECK_THROW(conn.query<tup>("select first_name, last_name from author where last_name = 'hodges';"),
                      system::system_error);
    conn.close();
}

struct author
{
  std::string last_name;
  std::string first_name;
};

#if __cplusplus < 202002L
BOOST_DESCRIBE_STRUCT(author, (), (last_name, first_name));
#endif

BOOST_AUTO_TEST_CASE(reflection)
{
        sqlite::connection conn;
    conn.connect(":memory:");
    conn.execute(
#include "test-db.sql"
    );


    bool found = false;
    for (author t : conn.query<author>("select first_name, last_name from author where last_name = 'hodges';"))
    {
      BOOST_CHECK(t.first_name == "richard");
      BOOST_CHECK(t.last_name == "hodges");
      found = true;
    }
    BOOST_CHECK(found);

    found = false;
    for (author t : conn.prepare("select first_name, last_name from author where last_name = ?;")
                        .execute<author>({"hodges"}))
    {
      BOOST_CHECK(t.first_name == "richard");
      BOOST_CHECK(t.last_name == "hodges");
      found = true;
    }
    BOOST_CHECK(found);

    BOOST_CHECK_THROW(conn.query<author>("select id, first_name, last_name from author where last_name = 'hodges';"),
    system::system_error);
    conn.close();
  }

