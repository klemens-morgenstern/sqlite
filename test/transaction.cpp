//
// Copyright (c) 2024 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/sqlite/transaction.hpp>

#include "test.hpp"

using namespace boost;

BOOST_AUTO_TEST_CASE(transaction)
{
  sqlite::connection conn{":memory:"};
  conn.execute("create table test(nr integer);");

  auto check_size = [&]{
    std::size_t n = 0ull;
    for (auto l : conn.query("select * from test"))
    {
      boost::ignore_unused(l);
      n++;
    }

    return n;
  };

  {
    sqlite::transaction t{conn};
    BOOST_CHECK_THROW(sqlite::transaction{conn}, system::system_error);

    conn.execute("insert into test values(1), (2)");
    BOOST_CHECK(check_size() == 2);

    {
      sqlite::savepoint sq{conn, "s1"};
      conn.execute("insert into test values(3)");
      BOOST_CHECK(check_size() == 3);
    }

    BOOST_CHECK(check_size() == 2);

    {
      sqlite::savepoint sq{conn, "s1"};
      conn.execute("insert into test values(4)");
      BOOST_CHECK(check_size() == 3);
      sq.commit();
    }

    BOOST_CHECK(check_size() == 3);

    {
      sqlite::savepoint sq{conn, "s1"};
      conn.execute("insert into test values(5)");
      BOOST_CHECK(check_size() == 4);
      {
        sqlite::savepoint sq2{conn, "s2"};
        conn.execute("insert into test values (6), (7)");
        BOOST_CHECK(check_size() == 6);
        sq2.commit();
      }
      BOOST_CHECK_EQUAL(check_size(), 6u);
    }
    BOOST_CHECK_EQUAL(check_size(), 3u);

  }

  BOOST_CHECK(conn.query("select * from test").done());

  {
    system::error_code ec;
    sqlite::error_info ei;
    conn.execute("BEGIN", ec, ei);
    BOOST_CHECK(!ec);
    conn.execute("BEGIN", ec, ei);
    BOOST_CHECK(ec);

    BOOST_CHECK_THROW(sqlite::transaction{conn}, system::system_error);
    sqlite::transaction t{conn, sqlite::transaction::adopt_transaction};

    conn.execute("insert into test values (42), (3);");
    t.commit();
  }

  BOOST_CHECK_EQUAL(check_size(), 2);
}
