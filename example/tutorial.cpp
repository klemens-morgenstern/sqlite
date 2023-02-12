//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/sqlite.hpp>

#include <iostream>

using namespace boost;

int main(int argc, char *argv[])
{
  sqlite::connection conn{":memory:"};

  conn.execute(R"(
create table author (
    id         integer primary key autoincrement,
    first_name text,
    last_name  text
);
create table library(
    id      integer primary key autoincrement,
    name    text unique,
    author  integer references author(id)
);
)");

  conn.prepare("insert into author (first_name, last_name) values (?1, ?2), (?3, ?4), (?5, ?6), (?7, ?8)")
      .execute({"vinnie", "falco", "richard", "hodges", "ruben", "perez", "peter", "dimov"});


  {
    conn.query("begin transaction;");

    auto st = conn.prepare(R"(insert into library ("name", author) values ($library,
                           (select id from author where first_name = $fname and last_name = $lname)))");

    st.execute({{"library", "beast"},    {"fname", "vinnie"}, {"lname", "falco"}});
    st.execute({{"library", "mysql"},    {"fname", "ruben"},  {"lname", "perez"}});
    st.execute({{"library", "mp11"},     {"fname", "peter"},  {"lname", "dimov"}});
    st.execute({{"library", "variant2"}, {"fname", "peter"},  {"lname", "dimov"}});

    conn.query("commit;");
  }

  struct collect_libs
  {
    void step(std::string & name, span<sqlite::value, 1> args)
    {
      if (name.empty())
        name = args[0].get_text();
      else
        (name += ", ") += args[0].get_text();
    }
    std::string final(std::string & name) { return name; }
  };
  sqlite::create_aggregate_function(conn, "collect_libs", collect_libs{});

  for (auto q : conn.query("select first_name, collect_libs(name) from author inner join library l on author.id = l.author group by last_name"))
    std::cout << q.at(0u).get_text() << " authored " << q.at(1u).get_text() << std::endl;


  return 0;
}