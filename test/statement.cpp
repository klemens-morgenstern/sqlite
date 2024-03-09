//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/sqlite/connection.hpp>
#include "test.hpp"

#include <boost/json.hpp>
#include <boost/algorithm/string.hpp>

#include <unordered_map>


using namespace boost;


BOOST_AUTO_TEST_CASE(statement)
{
  sqlite::connection conn;
  conn.connect(":memory:");
#if SQLITE_VERSION_NUMBER >= 3020000
  std::unique_ptr<int> data{new int(42)};

  auto ip = data.get();
  auto q = conn.prepare("select $1;").execute(std::make_tuple(std::move(data)));
  sqlite::row r = q.current();
  BOOST_CHECK(r.size() == 1u);

  auto v = r.at(0).get_value();
  BOOST_CHECK(v.type() == sqlite::value_type::null);
  BOOST_CHECK(v.get_pointer<int>() != nullptr);
  BOOST_CHECK(v.get_pointer<int>() == ip);
  BOOST_CHECK(v.get_pointer<double>() == nullptr);
#endif
  BOOST_CHECK_THROW(conn.prepare("select * from nothing where name = $name;").execute({}), boost::system::system_error);
}


BOOST_AUTO_TEST_CASE(decltype_)
{
  sqlite::connection conn;
  conn.connect(":memory:");
  conn.execute(
#include "test-db.sql"
  );
  auto q = conn.prepare("select* from author;");

  BOOST_CHECK(boost::iequals(q.declared_type(0), "INTEGER"));
  BOOST_CHECK(boost::iequals(q.declared_type(1), "TEXT"));
  BOOST_CHECK(boost::iequals(q.declared_type(2), "TEXT"));

  BOOST_CHECK_THROW(conn.prepare("elect * from nothing;"), boost::system::system_error);
}


BOOST_AUTO_TEST_CASE(map)
{
  sqlite::connection conn;
  conn.connect(":memory:");
  conn.execute(
#include "test-db.sql"
  );
  auto q = conn.prepare("select * from author where first_name = $name;").execute({{"name", 42}});
  BOOST_CHECK_THROW(conn.prepare("select * from nothing where name = $name;").execute({{"n4ame", 123}}), boost::system::system_error);

  std::unordered_map<std::string, variant2::variant<int, std::string>> params = {{"name", 42}};
  q = conn.prepare("select * from author where first_name = $name;").execute(params);
  BOOST_CHECK_THROW(conn.prepare("select * from nothing where name = $name;").execute(params), boost::system::system_error);

  BOOST_CHECK_THROW(conn.prepare("elect * from nothing;"), boost::system::system_error);
}