//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/sqlite/collation.hpp>
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

TEST_CASE("collation")
{
  sqlite::connection conn(":memory:");
  conn.execute(
#include "test-db.sql"
  );
  sqlite::create_collation(conn, "length", collate_length{});

  std::vector<std::string> names;

  // language=sqlite
  for (auto r : conn.query("select first_name from author where first_name = 5 collate length order by last_name asc;"))
    names.emplace_back(r.at(0).get_text());

  std::vector<std::string> cmp = {"peter", "ruben"};
  CHECK(names == cmp);
}