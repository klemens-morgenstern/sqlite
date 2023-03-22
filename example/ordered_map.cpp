//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/sqlite.hpp>
#include <boost/container/flat_map.hpp>
#include <iostream>

using namespace boost;


struct ordered_map_cursor final : sqlite::vtab::cursor<sqlite::string_view>
{
  container::flat_map<std::string, std::string> &data;
  ordered_map_cursor(container::flat_map<std::string, std::string> &data) : data(data) {}
  bool inverse = false;

  using reverse_iterator = typename container::flat_map<std::string, std::string>::reverse_iterator;
  using const_iterator = typename container::flat_map<std::string, std::string>::const_iterator;

  const_iterator begin{data.begin()}, end{data.end()};

  sqlite::result<void> next() { if (inverse) end--; else begin++; return {};}

  sqlite::result<sqlite3_int64> row_id()
  {
    return {system::in_place_error, SQLITE_MISUSE,
            "this shouldn't be called, we're omitting the row id"};
  }
  sqlite::result<sqlite::string_view> column(int i, bool nochange)
  {
    auto & elem = inverse ? *std::prev(end) : *begin;

    if (i == 0)
      return elem.first;
    else
      return elem.second;
  }
  sqlite::result<void> filter(int idx, const char * idxStr, span<sqlite::value> values) override
  {
    if (idx != 0)
      inverse = true;

    for (int i = 0; i < values.size(); i ++)
    {
      auto txt = values[i].get_text();
      switch (idxStr[i])
      {
        case SQLITE_INDEX_CONSTRAINT_EQ:
        {
          auto nw = data.equal_range(txt);
          if (nw.first > begin)
            begin = nw.first;
          if (nw.second < end)
            end = nw.second;
        }

          break;
        case SQLITE_INDEX_CONSTRAINT_GT:
        {
          auto new_begin = data.find(txt);
          new_begin ++;
          if (new_begin > begin)
            begin = new_begin;
        }
          break;
        case SQLITE_INDEX_CONSTRAINT_GE:
        {
          auto new_begin = data.find(txt);
          if (new_begin > begin)
            begin = new_begin;
        }
          break;
        case SQLITE_INDEX_CONSTRAINT_LE:
        {
          auto new_end = data.find(txt);
          new_end++;
          if (new_end < end)
            end = new_end;

        }
          break;
        case SQLITE_INDEX_CONSTRAINT_LT:
        {
          auto new_end = data.find(txt);
          if (new_end < end)
            end = new_end;
        }
          break;
      }

    }
    return {};
  }

  bool eof() noexcept
  {
    return begin == end;
  }
};


struct map_impl
    : sqlite::vtab::table<ordered_map_cursor>,
      sqlite::vtab::modifiable

{
  container::flat_map<std::string, std::string> data;
  const char * declaration()
  {
    return R"(
          create table url(
              name text primary key unique not null,
              data text) WITHOUT ROWID;)";
  }
  enum indices // 32
  {
    no_index   = 0b00000000,
    equal      = 0b00000001,
    gt         = 0b00000100,
    ge         = 0b00001100,
    lt         = 0000010000,
    le         = 0b00110000,
    order_asc  = 0b01000000,
    order_desc = 0b10000000,
  };


  sqlite::result<cursor_type> open()
  {
    return cursor_type{data};
  }

  sqlite::result<void> delete_(sqlite::value key)
  {
    data.erase(key.get_text());
    return {};
  }
  sqlite::result<sqlite_int64> insert(sqlite::value key, span<sqlite::value> values,
                                      int on_conflict)
  {
    data.emplace(values[0].get_text(), values[1].get_text());
    return 0;
  }

  sqlite::result<sqlite_int64> update(sqlite::value old_key, sqlite::value new_key,
                                      span<sqlite::value> values,
                                      int on_conflict)
  {
    if (new_key.get_int() != old_key.get_int())
      data.erase(old_key.get_text());
    data.insert_or_assign(values[0].get_text(), values[1].get_text());
    return 0;
  }


  sqlite::result<void> best_index(sqlite::vtab::index_info & info) override
  {
    // we're using the index to encode the mode, because it's simple enough.
    // more complex application should use it as an index like intended

    int idx = 0;
    sqlite::unique_ptr<char[]> str;
    if (info.constraints().size() > 0)
    {
      const auto sz = info.constraints().size()+1;
      str.reset(static_cast<char*>(sqlite3_malloc(sz)));
      std::memset(str.get(), '\0', sz);
    }
    else
      return {};

    for (auto i = 0u; i < info.constraints().size(); i++)
    {
      if ((idx & equal) != 0)
        break;
      auto & ct = info.constraints()[i];
      if (ct.iColumn == 0
          && ct.usable != 0) // aye, that's us
      {
        switch (ct.op)
        {
          // we'll stick to these
          case SQLITE_INDEX_CONSTRAINT_EQ: BOOST_FALLTHROUGH;
          case SQLITE_INDEX_CONSTRAINT_GT: BOOST_FALLTHROUGH;
          case SQLITE_INDEX_CONSTRAINT_GE: BOOST_FALLTHROUGH;
          case SQLITE_INDEX_CONSTRAINT_LE: BOOST_FALLTHROUGH;
          case SQLITE_INDEX_CONSTRAINT_LT:
            str[idx] = ct.op;
            info.usage()[i].argvIndex = ++idx; // use it -> value in this position in `filter`.
            info.usage()[i].omit = 1; // tell sqlite that we're sure enough, so sqlite doesn't check
            break;
          default:
            break;
        }
      }
    }


    if (info.order_by().size() == 1 && info.order_by()[0].iColumn == 0)
    {
      idx |= info.order_by()[0].desc != 0 ? order_desc : order_asc;
      info.set_already_ordered();
    }

    info.set_index(idx);
    if (str)
      info.set_index_string(str.release(), true);

    return {};
  }
};

struct ordered_map_module final : sqlite::vtab::eponymous_module<map_impl>
{

  sqlite::result<map_impl> connect(
      sqlite::connection conn, int argc, const char * const *argv)
  {
    return map_impl{};
  }
};


std::initializer_list<std::tuple<std::string, std::string>> raw_data = {
    {"atomic",                    "1.53.0"},
    {"chrono",                    "1.47.0"},
    {"container",                 "1.48.0"},
    {"context",                   "1.51.0"},
    {"contract",                  "1.67.0"},
    {"coroutine",                 "1.53.0"},
    {"date_time",                 "1.29.0"},
    {"exception",                 "1.36.0"},
    {"fiber",                     "1.62.0"},
    {"filesystem",                "1.30.0"},
    {"graph",                     "1.18.0"},
    {"graph_parallel",            "1.40.0"},
    {"headers",                   "1.00.0"},
    {"iostreams",                 "1.33.0"},
    {"json",                      "1.75.0"},
    {"locale",                    "1.48.0"},
    {"log",                       "1.54.0"},
    {"math",                      "1.23.0"},
    {"mpi",                       "1.35.0"},
    {"nowide",                    "1.73.0"},
    {"program_options",           "1.32.0"},
    {"python",                    "1.19.0"},
    {"random",                    "1.15.0"},
    {"regex",                     "1.18.0"},
    {"serialization",             "1.32.0"},
    {"stacktrace",                "1.65.0"},
    {"system",                    "1.35.0"},
    {"test",                      "1.21.0"},
    {"thread",                    "1.25.0"},
    {"timer",                     "1.9.0"},
    {"type_erasure",              "1.54.0"},
    {"url",                       "1.81.0"},
    {"wave",                      "1.33.0"}
};

void print(std::ostream & os, sqlite::resultset rw)
{
  os << "[";
  for (auto & r : rw)
    os << r.at(0).get_text() << ", ";
  os << "]" << std::endl;
}

int main (int argc, char * argv[])
{
  sqlite::connection conn{":memory:"};
  auto & m = sqlite::create_module(conn, "my_map", ordered_map_module());

  {
    auto p = conn.prepare("insert into my_map (name, data) values (?, ?);");
    for (const auto & d : raw_data)
      p.execute(d);
  }

  print(std::cout, conn.query("select * from my_map order by name desc;"));
  print(std::cout, conn.query("select * from my_map where name = 'url';"));
  print(std::cout, conn.query("select * from my_map where name < 'url' and name >= 'system' ;"));
  print(std::cout, conn.query("select * from my_map where name >  'json';"));
  print(std::cout, conn.query("select * from my_map where name >= 'json';"));
  print(std::cout, conn.query("select * from my_map where name <  'json';"));
  print(std::cout, conn.query("select * from my_map where name == 'json' order by name  asc;"));
  print(std::cout, conn.query("select * from my_map where name == 'json' order by name desc;"));

  print(std::cout, conn.query("select * from my_map where name < 'url' and name >= 'system' order by name desc;"));
  print(std::cout, conn.query("select * from my_map where data == '1.81.0';"));
  conn.query("delete from my_map where data == '1.81.0';");


  return 0;
}