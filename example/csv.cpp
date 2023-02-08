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

using namespace boost;

// This example demonstrates how to use the vtable interface - the csv part is inefficient.

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

// The implementation is very inefficient. Don't use this in product.
struct csv_impl
{
  struct table_type
  {
    std::string path;

    csv_data data;
    csv_data transaction_copy; // yeaup, inefficient, too.


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
    using iterator_type = typename container::flat_map<sqlite3_int64, csv_data::row_type>::const_iterator;
    struct cursor
    {
      iterator_type itr, end;
      void next() {itr++;}
      sqlite3_int64 row_id()
      {
        return itr->first;
      }

      sqlite::string_view column(int i, bool /* no_change */)
      {
          return itr->second.at(i);
      }

      bool eof() noexcept {return itr == end;}
    };

    cursor open()
    {
      return cursor{data.rows.cbegin(), data.rows.cend()};
    }

    void delete_(sqlite::value key)
    {
      data.rows.erase(key.get_int64());

    }
    sqlite_int64 insert(sqlite::value key, span<sqlite::value> values)
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
    sqlite_int64 update(sqlite::value update, sqlite::value new_key, span<sqlite::value> values)
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
    void begin()    {transaction_copy = data;}
    void sync()     {}
    void commit()
    {
      // ok, let's write to disk.
      //fs.(0);
      std::ofstream fs{path, std::fstream::trunc};
      fs << data << std::flush;
    }
    void rollback()
    {
      data = std::move(transaction_copy);
    }

    void destroy()
    {
        std::remove(path.c_str());
    }

  };

  table_type create(int argc, const char * const  argv[])
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



  table_type connect(int argc, const char * const  argv[])
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


int main (int argc, char * argv[])
{
  sqlite::connection conn{"./csv-example.db"};
  sqlite::create_module(conn, "csv_file", csv_impl());


  const auto init = !conn.has_table("csv_example");
  if (init)
    conn.execute("CREATE VIRTUAL TABLE if not exists csv_example USING csv_file(./csv-example.csv, username, first_name, last_name);");

  {
    conn.execute("begin");
    auto p = conn.prepare("insert into csv_example values (?, ?, ?)");
    if (init)
      p.execute({"anathal", "ruben",    "perez"});

    p.execute({"pdimov",              "peter",    "dimov"});
    p.execute({"klemens-morgenstern", "klemens",  "morgenstern"});

    if (init)
      p.execute({"richard", "hodges",   "madmongo1"});

    conn.execute("commit");
  }

  conn.query("delete from csv_example where first_name in ('peter', 'klemens')");
  return 0;
}
