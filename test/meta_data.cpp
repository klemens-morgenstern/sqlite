// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/sqlite/meta_data.hpp>
#include <boost/sqlite/connection.hpp>
#include "test.hpp"

using namespace boost;

TEST_CASE("meta-data")
{
  sqlite::connection conn;
  conn.connect(":memory:");
  conn.execute(
#include "test-db.sql"
  );

  CHECK(conn.changes() > 0u);
  CHECK(conn.total_changes() > 0u);


  auto fn = table_column_meta_data(conn,  "author", "first_name");
  CHECK( fn.data_type == "TEXT");
  CHECK(!fn.auto_increment);
  CHECK( fn.collation == "BINARY");
  CHECK(!fn.primary_key);
  CHECK( fn.not_null);

  auto ln = table_column_meta_data(conn, "main", "author", "last_name");
  CHECK( ln.data_type == "TEXT");
  CHECK(!ln.auto_increment);
  CHECK( ln.collation == "BINARY");
  CHECK(!ln.primary_key);
  CHECK(!ln.not_null);

  auto id = table_column_meta_data(conn, "main", "author", "id");
  CHECK( id.data_type == "INTEGER");
  CHECK( id.auto_increment);
  CHECK( id.collation == "BINARY");
  CHECK( id.primary_key);
  CHECK( id.not_null);

  conn.close();
}