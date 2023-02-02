// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/sqlite/function.hpp>
#include <boost/sqlite/connection.hpp>
#include "test.hpp"

using namespace boost;

TEST_CASE("scalar")
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
  CHECK(nm == names);
}

TEST_CASE("aggregate")
{
  sqlite::connection conn(":memory:");
  conn.execute(
#include "test-db.sql"
  );

  struct aggregate_func
  {
      std::size_t counter;
      void step(std::size_t & counter, boost::span<sqlite::value, 1u> val)
      {
        counter += val[0].get_text().size();
      }

      std::size_t final(std::size_t & counter)
      {
        return counter;
      }
  };

  sqlite::create_aggregate_function(
      conn,
      "char_counter",
      aggregate_func{});

  std::vector<std::size_t> lens;

  // language=sqlite
  for (auto r : conn.query("select char_counter(first_name) from author;"))
    lens.emplace_back(r.at(0).get_int64());

  CHECK(lens.size() == 1u);
  CHECK(lens[0]  == (5 + 6 + 7 + 5));
}


TEST_CASE("window")
{
  sqlite::connection conn(":memory:");
  conn.execute(
#include "test-db.sql"
  );

  struct window_func
  {
    std::size_t counter;
    void step(std::size_t & counter, boost::span<sqlite::value, 1u> val)
    {
      counter += val[0].get_text().size();
    }

    void inverse(std::size_t & counter, boost::span<sqlite::value, 1u> val)
    {
      counter -= val[0].get_text().size();
    }


    std::size_t value(std::size_t & counter)
    {
      return counter;
    }
  };

  sqlite::create_window_function(
      conn,
      "win_counter",
      window_func{});

  std::vector<std::size_t> lens;

  // language=sqlite
  for (auto r : conn.query(R"(
select win_counter(first_name) over (
  order by last_name rows between 1 preceding and 1 following ) as subrows
    from author order by last_name asc;)"))
    lens.emplace_back(r.at(0).get_int64());

  CHECK(lens.size() == 4u);
  CHECK(lens[0] == 11);
  CHECK(lens[1] == 18);
  CHECK(lens[2] == 18);
  CHECK(lens[3] == 12);
}