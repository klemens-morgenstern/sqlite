//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/sqlite.hpp>
#include <boost/describe/class.hpp>

#include <iostream>

using namespace boost;

// tag::query_t[]
struct users { std::string name; std::int64_t age; };
BOOST_DESCRIBE_STRUCT(users, (), (name, age));
// end::query_t[]

int main(int /*argc*/, char */*argv*/[])
{
#if defined(SQLITE_EXAMPLE_USE_DB)
// tag::conn_path[]
  sqlite::connection conn{"./my_db.db"};
// end::conn_path[]
#else
// tag::conn_mem[]
  sqlite::connection conn{sqlite::in_memory};
// end::conn_mem[]
#endif
  // tag::execute[]
  conn.execute(R"(
    CREATE TABLE IF NOT EXISTS users (
       id INTEGER PRIMARY KEY AUTOINCREMENT,
       name TEXT NOT NULL,
       age INTEGER NOT NULL);
    INSERT INTO users(name, age) VALUES('Alice', 30);
    INSERT INTO users(name, age) VALUES('Bob', 25);
  )");
  // end::execute[]

  // tag::query1[]
  sqlite::resultset q = conn.query("SELECT name, age FROM users ORDER BY id ASC;");
  // end::query1[]

  // tag::query2[]
  assert(q.current()[0].get_text() == "Alice");
  assert(q.read_next()); // true if it's the last row!
  assert(q.current()[0].get_text() == "Bob");
  // end::query2[]


  // tag::query3[]
  for (const auto & t :
          conn.query<std::tuple<sqlite::string_view, std::int64_t>>("SELECT name, age FROM users;"))
    std::cout << "User " << std::get<0>(t) << " is " << std::get<1>(t) << " old." << std::endl;
  // end::query3[]

  // tag::query4[]
  for (const auto & a : conn.query<users>("SELECT age, name FROM users;"))
    std::cout << "User " << a.name << " is " << a.age << " old." << std::endl;
  // end::query4[]

  // tag::query_strict[]
  for (const auto & a : conn.query<users>("SELECT age, name FROM users;").strict())
    std::cout << "User " << a.name << " is " << a.age << " old." << std::endl;
  // end::query_strict[]


  // tag::statement_insert[]
  conn.prepare("insert into users (name, age) values (?1, ?2), (?3, ?4)")
      .execute({"Paul", 31, "Mark", 51});
  // end::statement_insert[]


  // tag::statement_multi_insert[]
  {
    sqlite::transaction t{conn}; // use a transaction to speed this up

    auto st = conn.prepare(R"(insert into users ("name", age) values ($name, $age))");

    st.execute({{"name", "Allen"}, {"age", 43}});
    st.execute({{"name", "Tom"},   {"age", 84}});

    t.commit();
  }
  // end::statement_multi_insert[]

  // tag::to_upper[]
  sqlite::create_scalar_function(
      conn,
      "to_upper",
      [](sqlite::context<>, // <1>
             boost::span<sqlite::value, 1u> val // <2>
             ) -> std::string
      {
        if (val[0].type() != sqlite::value_type::text)
          throw std::logic_error("Value must be string"); // <3>
        auto txt = val[0].get_text();
        std::string res;
        res.resize(txt.size());
        std::transform(txt.begin(), txt.end(), res.begin(), [](char c){return std::toupper(c);});
        return res;
      },
      sqlite::deterministic // <4>
      );

  auto qu = conn.query("SELECT to_upper(name) FROM users WHERE name == 'Alice';");
  assert(qu.current()[0].get_text() == "ALICE");
  // end::to_upper[]


  // tag::oldest[]
  struct retirees
  {
    std::int64_t retirement_age;
    std::int64_t count = 0u;
    retirees(std::size_t retirement_age)
      : retirement_age(retirement_age) {}


    void step(span<sqlite::value, 1> args)
    {
      if (args[0].get_int() >= retirement_age)
        count += 1;
    }
    std::int64_t final() { return count; }
  };
  sqlite::create_aggregate_function<retirees>(conn, "retirees", std::make_tuple(65));

  q = conn.query("select retirees(age) from users;");
  std::cout << "The amount of retirees is " << q.current()[0].get_text() << std::endl;
  // end::oldest[]

  return 0;
}