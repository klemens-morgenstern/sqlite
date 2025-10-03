//
// Copyright (c) 2025 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/sqlite/connection.hpp>
#include <boost/sqlite/query.hpp>
#include "test.hpp"

#include <boost/json.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/describe/class.hpp>

#include <unordered_map>


using namespace boost;

struct author { std::string first_name, last_name;};
BOOST_DESCRIBE_STRUCT(author, (), (first_name, last_name));


BOOST_AUTO_TEST_CASE(row_query)
{
  sqlite::connection conn;
  conn.connect(":memory:");
  conn.execute(
#include "test-db.sql"
  );

  for (auto r : sqlite::query(conn, "select first_name, last_name from author where first_name = $name;", {{"name", "peter"}}))
  {
    BOOST_CHECK_EQUAL(r.at(0).get_text(), "peter");
    BOOST_CHECK_EQUAL(r.at(1).get_text(), "dimov");
  }

  
  for (auto r : sqlite::query<std::tuple<std::string, std::string>>(conn, "select first_name, last_name from author where first_name = $name;", {{"name", "peter"}}))
  {
    BOOST_CHECK_EQUAL(std::get<0>(r), "peter");
    BOOST_CHECK_EQUAL(std::get<1>(r), "dimov");
  }


  for (auto r : sqlite::query<author>(conn, "select first_name, last_name from author where first_name = $name;", {{"name", "peter"}}))
  {
    BOOST_CHECK_EQUAL(r.first_name, "peter");
    BOOST_CHECK_EQUAL(r.last_name,  "dimov");
  }

}

