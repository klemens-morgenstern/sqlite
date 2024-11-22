// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/sqlite/connection.hpp>
#include "test.hpp"

#include <filesystem>

using namespace boost;


BOOST_AUTO_TEST_CASE(connection)
{
  sqlite::connection conn;
  conn.connect(std::filesystem::path(":memory:"));
  conn.execute(
#include "test-db.sql"
  );

  BOOST_CHECK_THROW(conn.execute("elect * from nothing;"), boost::system::system_error);
  conn.close();
}

BOOST_AUTO_TEST_CASE(exc)
{
  sqlite::connection conn;
  conn.connect(sqlite::in_memory);
  BOOST_CHECK_THROW(conn.execute("select 932 fro 12;"), boost::system::system_error);
  conn.close();
}