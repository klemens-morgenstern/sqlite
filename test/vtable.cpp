//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "doctest.h"
#include "test.hpp"

#include <iostream>
#include <boost/sqlite.hpp>
#include <boost/unordered_map.hpp>
#include <boost/intrusive/list.hpp>

#include <string>
#include <vector>

using namespace boost;


TEST_SUITE_BEGIN("vtable");

struct simple_test_impl
{
  std::vector<std::string> names = {"ruben", "vinnie", "richard", "klemens"};

  struct table_type
  {
    constexpr static const char * declaration()
    {
      return R"(create table x(name text);)";
    }

    std::vector<std::string> & names;


    struct cursor
    {
      std::vector<std::string>::const_iterator  itr, end;

      void next() {itr++;}
      sqlite3_int64 row_id() {return *reinterpret_cast<sqlite3_int64*>(&itr);}

      core::string_view column(int i, bool /* no_change */)
      {
        if (i > 0)
          throw_exception(std::out_of_range("column out of range"));

        return *itr;
      }

      bool eof() noexcept {return itr == end;}
    };

    cursor open()
    {
      return cursor{names.begin(), names.end()};
    }

  };


  table_type connect(int argc, const char * const  argv[])
  {
    return table_type{names};
  }

};


TEST_CASE("simple reader")
{
  sqlite::connection conn(":memory:");
  auto & m = create_module(conn, "test_table", simple_test_impl{});

  auto itr = m.names.begin();
  for (auto q : conn.query("select * from test_table ;"))
  {
    CHECK(q.size() == 1);
    CHECK(q.at(0).get_text() == *itr++);
  }

  m.names.emplace_back("marcelo");
  itr = m.names.begin();
  for (auto q : conn.query("select * from test_table ;"))
  {
    CHECK(q.size() == 1);
    CHECK(q.at(0).get_text() == *itr++);
  }

  CHECK_THROWS(conn.query("insert into test_table values('chris')"));
}


struct modifyable_test_impl
{
  struct table_type : intrusive::list_base_hook<intrusive::link_mode<intrusive::auto_unlink> >
  {
    constexpr static const char * declaration()
    {
      return R"(create table x(id integer primary key autoincrement, name text);)";
    }

    std::string name;
    boost::unordered_map<int, std::string> names;

    int last_index = 0;

    table_type() = default;
    table_type(table_type && lhs) : name(std::move(lhs.name)), names(std::move(lhs.names)), last_index(lhs.last_index )
    {
      this->swap_nodes(lhs);
    }

    struct cursor
    {
      boost::unordered_map<int, std::string>::const_iterator  itr, end;

      void next() {itr++;}
      sqlite3_int64 row_id() {return itr->first;}

      variant2::variant<int, core::string_view> column(int i, bool /* no_change */)
      {
        switch (i)
        {
          case 0: return itr->first;
          case 1: return itr->second;
          default:
            throw_exception(std::out_of_range("column out of range"));

        }

        CHECK(sqlite::get_vtable<table_type>(this).name == "test_table");
      }

      bool eof() noexcept {return itr == end;}
    };

    cursor open()
    {
      return cursor{names.begin(), names.end()};
    }

    void delete_(sqlite::value key)
    {
      CHECK(names.erase(key.get_int()) == 1);
    }
    sqlite_int64 insert(sqlite::value key, span<sqlite::value> values,
                        int on_conflict)
    {
      int id = values[0].is_null() ? last_index++ : values[0].get_int();
      auto itr = names.emplace(id, values[1].get_text()).first;
      return itr->first;
    }
    sqlite_int64 update(sqlite::value old_key, sqlite::value new_key,
                        span<sqlite::value> values,
                        int on_conflict)
    {
      if (new_key.get_int() != old_key.get_int())
        names.erase(old_key.get_int());
      names.insert_or_assign(new_key.get_int(), values[1].get_text());
      return 0u;
    }
  };
  intrusive::list<table_type, intrusive::constant_time_size<false>> list;

  table_type connect(int argc, const char * const  argv[])
  {
    table_type tt{};
    tt.name.assign(argv[2]);
    list.push_back(tt);
    return tt;
  }
};


TEST_CASE("modifyable reader")
{
  sqlite::connection conn(":memory:");

  auto & m = create_module(conn, "name_table", modifyable_test_impl{});
  conn.execute("create virtual table test_table USING name_table; ");

  CHECK(m.list.size() == 1);
  CHECK(m.list.front().name == "test_table");
  CHECK(m.list.front().names.empty());

  conn.prepare("insert into test_table(name) values ($1), ($2), ($3), ($4);")
      .execute(std::make_tuple("vinnie", "richard", "ruben", "peter"));

  auto & t = m.list.front();
  CHECK(t.names.size() == 4);
  CHECK(t.names[0] == "vinnie");
  CHECK(t.names[1] == "richard");
  CHECK(t.names[2] == "ruben");
  CHECK(t.names[3] == "peter");

  conn.prepare("delete from test_table where name = $1;").execute(std::make_tuple("richard"));

  CHECK(t.names.size() == 3);
  CHECK(t.names[0] == "vinnie");
  CHECK(t.names[2] == "ruben");
  CHECK(t.names[3] == "peter");

  conn.prepare("update test_table set name = $1 where id = 0;").execute(std::make_tuple("richard"));

  CHECK(t.names.size() == 3);
  CHECK(t.names[0] == "richard");
  CHECK(t.names[2] == "ruben");
  CHECK(t.names[3] == "peter");
}