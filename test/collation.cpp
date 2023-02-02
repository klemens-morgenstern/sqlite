//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/sqlite/connection.hpp>
#include <boost/sqlite/collation.hpp>
#include <boost/sqlite/function.hpp>
#include <boost/sqlite/hooks.hpp>
#include <boost/sqlite/json.hpp>
#include <boost/callable_traits.hpp>

#include "doctest.h"

using namespace boost::sqlite;

struct aggregate_function
{
    struct my_ctx
    {
      int i = 42;
    };

    void step(my_ctx & ctx, boost::span<value> args)
    {

    }

    int final(my_ctx & ctx)
    {
      return 42;
    }
};

TEST_CASE("collation")
{
  connection conn(":memory:");

  CHECK_NOTHROW(conn.query("create table users (id integer primary key autoincrement, name text)"));
  CHECK_NOTHROW(conn.query("insert into users (name) values ('dick'), ('head')"));
  for (auto r : conn.query("select 42 as i, json_object('ex','[52,3.14159]') as js, json_array(1,2,'3',4) as ja;"))
  {
    for (auto c : r)
    {
      printf("Name: %s, Type %d, sub-type %d\n", c.column_name().data(), c.type(), c.get_value().subtype());
      printf("Data: %s\n", c.get_text().data());
    }
  }
}