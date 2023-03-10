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


  auto fn = table_column_meta_data(conn,  "author", "first_name");
  CHECK_MESSAGE(doctest::String(fn.data_type.data(), fn.data_type.size()).compare("TEXT", true) == 0,
                fn.data_type);
  CHECK(!fn.auto_increment);
  CHECK_MESSAGE(doctest::String(fn.collation.data(), fn.collation.size()).compare("BINARY", true) == 0,
                fn.collation);
  CHECK(!fn.primary_key);
  CHECK( fn.not_null);

  auto ln = table_column_meta_data(conn, "main", "author", "last_name");
  CHECK_MESSAGE(doctest::String(ln.data_type.data(), ln.data_type.size()).compare("TEXT", true)  == 0,
                ln.data_type);
  CHECK(!ln.auto_increment);
  CHECK_MESSAGE(doctest::String(ln.collation.data(), fn.collation.size()).compare("BINARY", true)  ==0,
                ln.collation);
  CHECK(!ln.primary_key);
  CHECK(!ln.not_null);

  auto id = table_column_meta_data(conn, "main", "author", "id");
  CHECK(doctest::String(id.data_type.data(), id.data_type.size()).compare("INTEGER", true) == 0);
  CHECK( id.auto_increment);
  CHECK(doctest::String(id.collation.data(), fn.collation.size()).compare("BINARY", true) == 0);
  CHECK( id.primary_key);
  CHECK( id.not_null);

  conn.close();
}