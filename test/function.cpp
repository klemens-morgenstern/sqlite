// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/sqlite/function.hpp>
#include <boost/sqlite/connection.hpp>
#include "test.hpp"

#include <string>
#include <vector>

using namespace boost;

BOOST_AUTO_TEST_CASE(scalar)
{
  sqlite::connection conn(":memory:");
  conn.execute(
#include "test-db.sql"
  );

  sqlite::create_scalar_function(
      conn,
      "to_upper",
      [](sqlite::context<>, boost::span<sqlite::value, 1u> val)
              -> variant2::variant<variant2::monostate, sqlite::value, std::string>
      {
        if (val.empty())
          return {};

        if (val[0].type() != sqlite::value_type::text)
          return val[0];

        auto txt = val[0].get_text();
        std::string res;
        res.resize(txt.size());
        std::transform(txt.begin(), txt.end(), res.begin(), [](char c){return std::toupper(c);});
        return res;
      });

  std::vector<std::string> names;

  // language=sqlite
  for (auto r : conn.query("select to_upper(first_name) from author order by last_name asc;"))
    names.emplace_back(r.at(0).get_text());


  std::vector<std::string> nm = {"PETER", "VINNIE", "RICHARD", "RUBEN"};
  BOOST_CHECK(nm == names);
}


BOOST_AUTO_TEST_CASE(scalar_pointer)
{
  sqlite::connection conn(":memory:");
  conn.execute(
#include "test-db.sql"
  );

  sqlite::create_scalar_function(
      conn,
      "to_upper",
      +[](sqlite::context<>, boost::span<sqlite::value, 1u> val)
              -> variant2::variant<variant2::monostate, sqlite::value, std::string>
      {
        if (val.empty())
          return {};

        if (val[0].type() != sqlite::value_type::text)
          return val[0];

        auto txt = val[0].get_text();
        std::string res;
        res.resize(txt.size());
        std::transform(txt.begin(), txt.end(), res.begin(), [](char c){return std::toupper(c);});
        return res;
      });

  std::vector<std::string> names;

  // language=sqlite
  for (auto r : conn.query("select to_upper(first_name) from author order by last_name asc;"))
    names.emplace_back(r.at(0).get_text());


  std::vector<std::string> nm = {"PETER", "VINNIE", "RICHARD", "RUBEN"};
  BOOST_CHECK(nm == names);
}


BOOST_AUTO_TEST_CASE(scalar_void)
{
  sqlite::connection conn(":memory:");
  conn.execute(
#include "test-db.sql"
  );

  sqlite::create_scalar_function(
      conn,
      "to_upper",
      [](sqlite::context<>, boost::span<sqlite::value, 1u> val)
      {
        return ;
      });


  // language=sqlite
  for (auto r : conn.query("select to_upper(first_name) from author order by last_name asc;"))
    BOOST_CHECK(r[0].is_null());
}


BOOST_AUTO_TEST_CASE(scalar_void_pointer)
{
  sqlite::connection conn(":memory:");
  conn.execute(
#include "test-db.sql"
  );

  sqlite::create_scalar_function(
      conn,
      "to_upper",
      +[](sqlite::context<>, boost::span<sqlite::value, 1u> val)
      {
        return ;
      });

  std::vector<std::string> names;

  // language=sqlite
  for (auto r : conn.query("select to_upper(first_name) from author order by last_name asc;"))
    BOOST_CHECK(r[0].is_null());
}



BOOST_AUTO_TEST_CASE(aggregate)
{
  sqlite::connection conn(":memory:");
  conn.execute(
#include "test-db.sql"
  );

  struct aggregate_func
  {
      aggregate_func(int value) : counter(value) {}
      std::size_t counter;
      void step(boost::span<sqlite::value, 1u> val)
      {
        counter += val[0].get_text().size();
      }

      std::size_t final()
      {
        return counter;
      }
  };

  sqlite::create_aggregate_function<aggregate_func>(
      conn,
      "char_counter", std::make_tuple(0));

  std::vector<std::size_t> lens;

  // language=sqlite
  for (auto r : conn.query("select char_counter(first_name) from author;"))
    lens.emplace_back(r.at(0).get_int());

  BOOST_CHECK(lens.size() == 1u);
  BOOST_CHECK(lens[0]  == (5 + 6 + 7 + 5));
}

BOOST_AUTO_TEST_CASE(aggregate_result)
{
  sqlite::connection conn(":memory:");
  conn.execute(
#include "test-db.sql"
  );

  struct aggregate_func
  {
    aggregate_func(int value) : counter(value) {}
    std::size_t counter;
    sqlite::result<void> step(boost::span<sqlite::value, 1u> val)
    {
      counter += val[0].get_text().size();
      return {};
    }

    sqlite::result<std::size_t> final()
    {
      return counter;
    }
  };

  sqlite::create_aggregate_function<aggregate_func>(
      conn,
      "char_counter", std::make_tuple(0));

  std::vector<std::size_t> lens;

  // language=sqlite
  for (auto r : conn.query("select char_counter(first_name) from author;"))
    lens.emplace_back(r.at(0).get_int());

  BOOST_CHECK(lens.size() == 1u);
  BOOST_CHECK(lens[0]  == (5 + 6 + 7 + 5));
}

#if SQLITE_VERSION_NUMBER >= 3025000
BOOST_AUTO_TEST_CASE(window)
{
  sqlite::connection conn(":memory:");
  conn.execute(
#include "test-db.sql"
  );

  struct window_func
  {
    std::size_t counter;
    void step(boost::span<sqlite::value, 1u> val)
    {
      counter += val[0].get_text().size();
    }

    void inverse(boost::span<sqlite::value, 1u> val)
    {
      counter -= val[0].get_text().size();
    }


    std::size_t value()
    {
      return counter;
    }
  };

  sqlite::create_window_function<window_func>(
      conn,
      "win_counter");

  std::vector<std::size_t> lens;

  // language=sqlite
  for (auto r : conn.query(R"(
select win_counter(first_name) over (
  order by last_name rows between 1 preceding and 1 following ) as subrows
    from author order by last_name asc;)"))
    lens.emplace_back(r.at(0).get_int());

  BOOST_CHECK(lens.size() == 4u);
  BOOST_CHECK(lens[0] == 11);
  BOOST_CHECK(lens[1] == 18);
  BOOST_CHECK(lens[2] == 18);
  BOOST_CHECK(lens[3] == 12);
}

BOOST_AUTO_TEST_CASE(window_result)
{
  sqlite::connection conn(":memory:");
  conn.execute(
#include "test-db.sql"
  );

  struct window_func
  {
    std::size_t counter;
    sqlite::result<void> step(boost::span<sqlite::value, 1u> val)
    {
      counter += val[0].get_text().size();
      return{};
    }

    sqlite::result<void> inverse(boost::span<sqlite::value, 1u> val)
    {
      counter -= val[0].get_text().size();
      return {};
    }


    sqlite::result<std::size_t> value()
    {
      return counter;
    }
  };

  sqlite::create_window_function<window_func>(
      conn,
      "win_counter");

  std::vector<std::size_t> lens;

  // language=sqlite
  for (auto r : conn.query(R"(
select win_counter(first_name) over (
  order by last_name rows between 1 preceding and 1 following ) as subrows
    from author order by last_name asc;)"))
    lens.emplace_back(r.at(0).get_int());

  BOOST_CHECK(lens.size() == 4u);
  BOOST_CHECK(lens[0] == 11);
  BOOST_CHECK(lens[1] == 18);
  BOOST_CHECK(lens[2] == 18);
  BOOST_CHECK(lens[3] == 12);
}
#endif