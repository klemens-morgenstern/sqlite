// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/sqlite/hooks.hpp>
#include <boost/sqlite/connection.hpp>
#include "test.hpp"

using namespace boost;

TEST_CASE("hooks")
{
  sqlite::connection conn(":memory:");
  conn.execute(
#include "test-db.sql"
  );

  bool called = false;
  auto l =
      [&](int op, core::string_view db, core::string_view table, sqlite3_int64 id) noexcept
      {
        CHECK(op == SQLITE_INSERT);
        CHECK(db == "main");
        CHECK(table == "library");
        called = true;
      };


  sqlite::update_hook(conn, l);
  // language=sqlite
  conn.query(R"(
    insert into library ("name", "author") values
      ('mustache',(select id from author where first_name = 'peter'  and last_name = 'dimov'));
    )");

  CHECK(called);


}
