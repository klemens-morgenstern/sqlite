//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


#include <boost/sqlite/backup.hpp>
#include <boost/sqlite/connection.hpp>

#include <string>
#include <vector>

#include "test.hpp"

using namespace boost;

BOOST_AUTO_TEST_CASE(backup)
{
  sqlite::connection conn1{":memory:"};
  conn1.execute(
#include "test-db.sql"
      );
  // language=sqlite
  conn1.query("select * from author;");


  sqlite::connection conn2{":memory:"};
  sqlite::backup(conn1, conn2);

  std::vector<std::string> names1, names2;

  // language=sqlite
  for (auto r : conn1.query("select first_name from author;"))
    names1.emplace_back(r.at(0u).get_text());

  // language=sqlite
  for (auto r : conn2.query("select first_name from author;"))
    names2.emplace_back(r.at(0u).get_text());

  BOOST_CHECK(!names1.empty());
  BOOST_CHECK(!names2.front().empty());
  BOOST_CHECK(names1 == names2);
  BOOST_CHECK_THROW(sqlite::backup(conn1, conn2, "foo", "bar"), boost::system::system_error);
}