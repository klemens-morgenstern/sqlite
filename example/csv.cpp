//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/sqlite.hpp>
#include <boost/unordered_map.hpp>
#include <boost/describe.hpp>
#include <boost/variant2/variant.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <fstream>
#include <iostream>
#include <vector>

using namespace boost;

// This example demonstrates how to use the vtable interface to read & write from a csv file.
// The csv implementation is not efficient, but for demontration purposes.


struct csv_data
{
  using row_type = std::vector<std::string>;
  row_type names;
  container::flat_map<sqlite3_int64 , row_type> rows;
};

csv_data::row_type read_line(std::string line)
{
  csv_data::row_type row;
  std::string cell;
  std::istringstream is{line};
  while (std::getline(is, cell, ','))
  {
    boost::trim(cell);
    row.push_back(std::move(cell));
  }
  return row;
}


std::istream & operator>>(std::istream & is, csv_data &cs)
{
  cs.names.clear();
  cs.rows.clear();

  std::string line;
  if(std::getline(is, line))
    cs.names = read_line(std::move(line));

  sqlite3_int64 cnt = 1;
  while(std::getline(is, line))
    cs.rows.emplace(cnt++, read_line(std::move(line)));
  return is;
}


std::ostream & operator<<(std::ostream & os, csv_data &cs)
{

  for (auto & nm : cs.names)
  {
    os  << nm;
    if (&nm == &cs.names.back())
      os << '\n';
    else
      os << ", ";
  }

  for (auto & r : cs.rows)
    for (auto & nm : r.second)
    {
      os  << nm;
      if (&nm == &r.second.back())
        os << '\n';
      else
        os << ", ";
    }

  return os;
}


using iterator_type = typename container::flat_map<sqlite3_int64, csv_data::row_type>::const_iterator;

struct csv_cursor final : sqlite::vtab::cursor<sqlite::string_view>
{
  iterator_type itr, end;

  csv_cursor(iterator_type itr,
             iterator_type end) : itr(itr), end(end)
  {}

  sqlite::result<void> next() {itr++; return {};}
  sqlite::result<sqlite3_int64> row_id()
  {
    return itr->first;
  }

  sqlite::result<sqlite::string_view> column(int i, bool /* no_change */)
  {
    return itr->second.at(i);
  }

  bool eof() noexcept {return itr == end;}
};

struct csv_table final
    : sqlite::vtab::table<csv_cursor>,
      sqlite::vtab::modifiable,
      sqlite::vtab::transaction
{
  std::string path;
  csv_data data;
  csv_data transaction_copy; // yeaup, inefficient, too.

  csv_table(std::string path) : path(std::move(path))
  {}

  std::string decl;
  const char * declaration()
  {
    if (decl.empty())
    {
      std::ostringstream  oss;
      decl = "create table x(";

      for (auto & nm : data.names)
      {
        decl += nm;
        if (&nm == &data.names.back())
          decl += ");";
        else
          decl += ", ";
      }
    }
    return decl.c_str();
  }

  sqlite::result<cursor_type> open()
  {
    return cursor_type{data.rows.cbegin(), data.rows.cend()};
  }

  sqlite::result<void> delete_(sqlite::value key)
  {
    data.rows.erase(key.get_int64());
    return {};
  }
  sqlite::result<sqlite_int64> insert(
      sqlite::value /*key*/, span<sqlite::value> values, int /*on_conflict*/)
  {
    sqlite3_int64 id = 0;
    if (!data.rows.empty())
      id = std::prev(data.rows.end())->first + 1;
    auto & ref = data.rows[id];
    ref.reserve(values.size());
    for (auto v : values)
      ref.emplace_back(v.get_text());

    return id;
  }
  sqlite::result<sqlite_int64> update(
                      sqlite::value update, sqlite::value new_key,
                      span<sqlite::value> values, int /*on_conflict*/)
  {
    if (!new_key.is_null())
      throw std::logic_error("we can't manually set keys");

    int i = 0;
    auto & r = data.rows[update.get_int()];
    for (auto val : values)
      r[i].assign(val.get_text());

    return 0u;
  }

  // we do not read the csv , but just dump it on
  sqlite::result<void> begin()  noexcept   {transaction_copy = data; return {};}
  sqlite::result<void> sync()   noexcept   {return {};}
  sqlite::result<void> commit() noexcept
  {
    // ok, let's write to disk.
    //fs.(0);
    std::ofstream fs{path, std::fstream::trunc};
    fs << data << std::flush;
    return {};
  }
  sqlite::result<void> rollback()  noexcept
  {
    data = std::move(transaction_copy);
    return {};
  }

  sqlite::result<void> destroy()  noexcept
  {
    std::remove(path.c_str());
    return {};
  }
};

// The implementation is very inefficient. Don't use this in production.
struct csv_module final : sqlite::vtab::module<csv_table>
{

  sqlite::result<table_type> create(sqlite::connection /*db*/,
                                    int argc, const char * const  argv[])
  {
    if (argc < 4)
      throw std::invalid_argument("Need filename as first parameter");

    table_type tt{argv[3]};
    tt.data.names.reserve(argc - 4);
    for (int i = 4; i < argc; i++)
      tt.data.names.emplace_back(argv[i]);
    std::ofstream fs(tt.path, std::fstream::trunc);
    fs << tt.data << std::flush;
    return tt;
  }

  sqlite::result<table_type>  connect(sqlite::connection /*db*/,
                                      int argc, const char * const  argv[])
  {
    if (argc < 4)
      throw std::invalid_argument("Need filename as first parameter");

    table_type tt{argv[3]};
    tt.data.names.reserve(argc - 4);
    for (int i = 4; i < argc; i++)
      tt.data.names.emplace_back(argv[i]);
    std::ifstream fs(tt.path, std::fstream::in);
    // read the existing data
    fs >> tt.data;

    if (!std::equal(tt.data.names.begin(), tt.data.names.end(), argv+4, argv + argc))
      throw std::runtime_error("Column names in csv do not match");

    return tt;
  }
};


int main (int /*argc*/, char * /*argv*/[])
{
  sqlite::connection conn{"./csv-example.db"};
  sqlite::create_module(conn, "csv_file", csv_module());

  const auto empty_csv = !conn.has_table("csv_example");
  if (empty_csv)
    conn.execute("CREATE VIRTUAL TABLE if not exists csv_example USING csv_file(./csv-example.csv, username, first_name, last_name);");

  {
    conn.execute("begin");
    auto p = conn.prepare("insert into csv_example values (?, ?, ?)");
    if (empty_csv)
      p.execute({"anarthal", "ruben",    "perez"});

    p.execute({"pdimov",              "peter",    "dimov"});
    p.execute({"klemens-morgenstern", "klemens",  "morgenstern"});

    if (empty_csv)
      p.execute({"madmongo1", "richard", "hodges"});

    conn.execute("commit");
  }

  conn.query("delete from csv_example where first_name in ('peter', 'klemens')");
  return 0;
}
