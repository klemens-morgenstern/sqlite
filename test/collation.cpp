//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/sqlite/collation.hpp>
#include <boost/sqlite/iterator.hpp>
#include <string>
#include <vector>

#include "test.hpp"

using namespace boost;

struct collate_length
{
  int operator()(core::string_view l, core::string_view r) noexcept
  {
    return std::stoull(r) - l.size();
  }
};

BOOST_AUTO_TEST_CASE(collation)
{
  sqlite::connection conn(":memory:");
  conn.execute(
#include "test-db.sql"
  );

  sqlite::create_collation(conn, "length", collate_length{});

  std::vector<std::string> names;

  // language=sqlite
  auto sr = conn.prepare("select first_name from author where first_name = 5 collate length order by last_name asc;");
  for (auto r : sqlite::statement_range<sqlite::row>(sr))
    names.emplace_back(r.at(0).get_text());

  std::vector<std::string> cmp = {"peter", "ruben"};
  BOOST_CHECK(names == cmp);

  sqlite::delete_collation(conn, "length");

  BOOST_CHECK_THROW(conn.prepare("select first_name from author where first_name = 5 collate length order by last_name asc;"), system::system_error);
}
