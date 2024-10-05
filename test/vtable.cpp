//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/test/unit_test.hpp>
#include "test.hpp"

#include <iostream>
#include <boost/sqlite.hpp>
#include <boost/unordered_map.hpp>
#include <boost/intrusive/list.hpp>

#include <string>
#include <vector>

using namespace boost;


BOOST_AUTO_TEST_SUITE(vtable_);

struct del_info : sqlite3_index_info
{
  del_info() : sqlite3_index_info()
  {

  }

  ~del_info()
  {
    if (needToFreeIdxStr)
      sqlite3_free(idxStr);
  }
};

struct trivial_struct
{
  trivial_struct() = default;
  int i{1}, j{2}, k{3};
};



struct simple_cursor final : sqlite::vtab::cursor<core::string_view>
{

  simple_cursor(std::vector<std::string>::const_iterator itr,
                std::vector<std::string>::const_iterator end) : itr(itr), end(end) {}
  std::vector<std::string>::const_iterator  itr, end;

  sqlite::result<void> next() {itr++; return {};}
  sqlite::result<sqlite3_int64> row_id() {return *reinterpret_cast<sqlite3_int64*>(&itr);}

  sqlite::result<core::string_view> column(int i, bool /* no_change */)
  {
    if (i > 0)
      throw_exception(std::out_of_range("column out of range"));

    return *itr;
  }

  bool eof() noexcept {return itr == end;}
};

struct simple_table final : sqlite::vtab::table<simple_cursor>
{
  const char * declaration()
  {
    return R"(create table x(name text);)";
  }

  simple_table(std::vector<std::string> & names) : names(names) {}
  std::vector<std::string> & names;

  sqlite::result<cursor_type> open()
  {
    return cursor_type{names.begin(), names.end()};
  }

};

struct simple_test_impl final : sqlite::vtab::eponymous_module<simple_table>
{
  std::vector<std::string> names = {"ruben", "vinnie", "richard", "klemens"};




  sqlite::result<table_type> connect(sqlite::connection,
                                     int argc, const char * const  argv[])
  {
    return table_type{names};
  }

};


BOOST_AUTO_TEST_CASE(simple_reader)
{
  sqlite::connection conn(":memory:");
  auto & m = create_module(conn, "test_table", simple_test_impl{});

  auto itr = m.names.begin();
  for (auto q : conn.query("select * from test_table ;"))
  {
    BOOST_CHECK(q.size() == 1);
    BOOST_CHECK(q.at(0).get_text() == *itr++);
  }

  m.names.emplace_back("marcelo");
  itr = m.names.begin();
  for (auto q : conn.query("select * from test_table ;"))
  {
    BOOST_CHECK(q.size() == 1);
    BOOST_CHECK(q.at(0).get_text() == *itr++);
  }

  BOOST_CHECK_THROW(conn.query("insert into test_table values('chris')"), boost::system::system_error);
}

struct modifyable_table;

struct modifyable_cursor final : sqlite::vtab::cursor<variant2::variant<int, core::string_view>>
{
  using iterator = boost::unordered_map<int, std::string>::const_iterator;

  modifyable_cursor(iterator itr, iterator end) : itr(itr), end(end) {}

  iterator itr, end;

  sqlite::result<void> next()            noexcept {itr++; return {};}
  sqlite::result<sqlite3_int64> row_id() noexcept {return itr->first;}

  sqlite::result<column_type> column(int i, bool /* no_change */) noexcept
  {
    switch (i)
    {
      case 0: return itr->first;
      case 1: return itr->second;
      default:
        return sqlite::error(SQLITE_RANGE, sqlite::error_info("column out of range"));

    }
  }

  bool eof() noexcept {return itr == end;}
};

struct modifyable_table final :
    sqlite::vtab::table<modifyable_cursor>,
    intrusive::list_base_hook<intrusive::link_mode<intrusive::auto_unlink> >,
    sqlite::vtab::modifiable
{
  const char * declaration()
  {
    return R"(create table x(id integer primary key autoincrement, name text);)";
  }

  std::string name;
  boost::unordered_map<int, std::string> names;

  int last_index = 0;

  modifyable_table() = default;
  modifyable_table(modifyable_table && lhs)
    : name(std::move(lhs.name)), names(std::move(lhs.names)), last_index(lhs.last_index )
  {
    this->swap_nodes(lhs);
  }


  sqlite::result<modifyable_cursor> open()
  {
    return modifyable_cursor{names.begin(), names.end()};
  }

  sqlite::result<void> delete_(sqlite::value key)
  {
    BOOST_CHECK(names.erase(key.get_int()) == 1);
    return {};
  }
  sqlite::result<sqlite_int64> insert(sqlite::value key, span<sqlite::value> values,
                      int on_conflict)
  {
    int id = values[0].is_null() ? last_index++ : values[0].get_int();
    auto itr = names.emplace(id, values[1].get_text()).first;
    return itr->first;
  }
  sqlite::result<sqlite_int64> update(sqlite::value old_key, sqlite::value new_key,
                      span<sqlite::value> values,
                      int on_conflict)
  {
    if (new_key.get_int() != old_key.get_int())
      names.erase(old_key.get_int());
    names.insert_or_assign(new_key.get_int(), values[1].get_text());
    return 0u;
  }
};

struct modifyable_test_impl final : sqlite::vtab::eponymous_module<modifyable_table>
{
  modifyable_test_impl() = default;

  intrusive::list<table_type, intrusive::constant_time_size<false>> list;

  sqlite::result<table_type> connect(sqlite::connection,
                                     int argc, const char * const  argv[]) noexcept
  {
    table_type tt{};
    tt.name.assign(argv[2]);
    list.push_back(tt);
    return sqlite::result<table_type>(std::move(tt));
  }
};


BOOST_AUTO_TEST_CASE(modifyable_reader)
{
  sqlite::connection conn(":memory:");

  auto & m = create_module(conn, "name_table", modifyable_test_impl{});
  conn.execute("create virtual table test_table USING name_table; ");

  BOOST_CHECK(m.list.size() == 1);
  BOOST_CHECK(m.list.front().name == "test_table");
  BOOST_CHECK(m.list.front().names.empty());

  conn.prepare("insert into test_table(name) values ($1), ($2), ($3), ($4);")
      .execute(std::make_tuple("vinnie", "richard", "ruben", "peter"));

  auto & t = m.list.front();
  BOOST_CHECK(t.names.size() == 4);
  BOOST_CHECK(t.names[0] == "vinnie");
  BOOST_CHECK(t.names[1] == "richard");
  BOOST_CHECK(t.names[2] == "ruben");
  BOOST_CHECK(t.names[3] == "peter");

  conn.prepare("delete from test_table where name = $1;").execute(std::make_tuple("richard"));

  BOOST_CHECK(t.names.size() == 3);
  BOOST_CHECK(t.names[0] == "vinnie");
  BOOST_CHECK(t.names[2] == "ruben");
  BOOST_CHECK(t.names[3] == "peter");

  conn.prepare("update test_table set name = $1 where id = 0;").execute(std::make_tuple("richard"));

  BOOST_CHECK(t.names.size() == 3);
  BOOST_CHECK(t.names[0] == "richard");
  BOOST_CHECK(t.names[2] == "ruben");
  BOOST_CHECK(t.names[3] == "peter");
}


BOOST_AUTO_TEST_SUITE_END()
