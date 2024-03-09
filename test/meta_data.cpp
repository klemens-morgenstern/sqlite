// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/sqlite/meta_data.hpp>
#include <boost/sqlite/connection.hpp>
#include "test.hpp"
#include <boost/algorithm/string.hpp>

using namespace boost;

BOOST_AUTO_TEST_CASE(meta_data)
{
  sqlite::connection conn;
  conn.connect(":memory:");
  conn.execute(
#include "test-db.sql"
  );



  auto fn = table_column_meta_data(conn,  "author", "first_name");
  BOOST_CHECK_MESSAGE(boost::iequals(fn.data_type, "TEXT"), fn.data_type);
  BOOST_CHECK(!fn.auto_increment);
  BOOST_CHECK_MESSAGE(boost::iequals(fn.collation, "BINARY"), fn.collation);
  BOOST_CHECK(!fn.primary_key);
  BOOST_CHECK( fn.not_null);

  auto ln = table_column_meta_data(conn, "main", "author", "last_name");
  BOOST_CHECK_MESSAGE(boost::iequals(ln.data_type, "TEXT"), ln.data_type);
  BOOST_CHECK(!ln.auto_increment);
  BOOST_CHECK_MESSAGE(boost::iequals(ln.collation, "BINARY"), ln.collation);
  BOOST_CHECK(!ln.primary_key);
  BOOST_CHECK(!ln.not_null);

  auto id = table_column_meta_data(conn, "main", "author", "id");
  BOOST_CHECK(boost::iequals(id.data_type, "INTEGER"));
  BOOST_CHECK( id.auto_increment);
  BOOST_CHECK(boost::iequals(id.collation, "BINARY"));
  BOOST_CHECK( id.primary_key);
  BOOST_CHECK( id.not_null);

  conn.close();
}