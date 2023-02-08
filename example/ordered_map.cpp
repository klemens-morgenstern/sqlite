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


struct ordered_map
{
  constexpr static bool eponymous_only = false;

  struct map_impl
  {
    container::flat_map<std::string, std::string> data;
    constexpr static const char * declaration()
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

    struct cursor
    {
      container::flat_map<std::string, std::string> &data;
      sqlite3_index_info* last_info;
      bool inverse = false;

      using reverse_iterator = typename container::flat_map<std::string, std::string>::reverse_iterator;
      using const_iterator = typename container::flat_map<std::string, std::string>::const_iterator;

      const_iterator begin{data.begin()}, end{data.end()};

      void next() { if (inverse) end--; else begin++; }

      sqlite3_int64 row_id()
      {
        throw std::logic_error("this shouldn't be called, we're omitting the row id");
      }
      sqlite::string_view column(int i, bool nochange)
      {
        auto & elem = inverse ? *std::prev(end) : *begin;

        if (i == 0)
          return elem.first;
        else
          return elem.second;
      }
      void filter(int idx, const char * idxStr, span<sqlite::value> values)
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

      }

      bool eof() const noexcept
      {
        return begin == end;
      }
    };

    cursor open()
    {
      return cursor{data, last_info};
    }

    void delete_(sqlite::value key)
    {
      data.erase(key.get_text());
    }
    sqlite_int64 insert(sqlite::value key, span<sqlite::value> values)
    {
      data.emplace(values[0].get_text(), values[1].get_text());
      return 0;
    }

    sqlite_int64 update(sqlite::value old_key, sqlite::value new_key, span<sqlite::value> values)
    {
      if (new_key.get_int() != old_key.get_int())
        data.erase(old_key.get_text());
      data.insert_or_assign(values[0].get_text(), values[1].get_text());
      return 0;
    }

    sqlite3_index_info* last_info;


    void best_index(sqlite3_index_info* info)
    {
        // we're using the index to encode the mode, because it's simple enough.
        // more complex application should use it as an index like intended

        last_info = info;
        int idx = 0;


        if (info->nConstraint > 0)
        {
          info->idxStr = static_cast<char*>(sqlite3_malloc(info->nConstraint+1));
          std::memset(info->idxStr, '\0', info->nConstraint+1);
          info->needToFreeIdxStr = 1;
        }

        for (int i = 0; i < info->nConstraint; i++)
        {
          if ((info->idxNum & equal) != 0)
            break;
          auto & ct = info->aConstraint[i];
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
                info->idxStr[idx] = ct.op;
                info->aConstraintUsage[i].argvIndex = ++idx; // use it -> value in this position in `filter`.
                info->aConstraintUsage[i].omit = 1; // tell sqlite that we're sure enough, so sqlite doesn't check
                break;
              default:
                break;
            }
          }
        }


        if (info->nOrderBy == 1 && info->aOrderBy->iColumn == 0)
        {
          info->idxNum = 1;
          info->orderByConsumed = true;
        }



    }
  };

  map_impl connect(int argc, const char * const *argv)
  {
    return map_impl{};
  }
};


std::initializer_list<std::tuple<std::string, std::string>> data = {
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
  auto & m = sqlite::create_module(conn, "my_map", ordered_map());

  {
    auto p = conn.prepare("insert into my_map (name, data) values (?, ?);");
    for (const auto & d : data)
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