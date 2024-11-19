//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/sqlite.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/key.hpp>
#include <boost/optional.hpp>
#include <algorithm>
#include <iostream>

using namespace boost;

namespace mi = boost::multi_index;

struct boost_library
{
  std::string name, version;
};

using my_container = mi::multi_index_container<
    boost_library,
    mi::indexed_by<
        mi::ordered_unique<mi::key<&boost_library::name>>,
        mi::ordered_non_unique<mi::key<&boost_library::version>>
  >
>;


struct multi_index_cursor final
    : sqlite::vtab::cursor<sqlite::string_view>
{
  my_container &data;

  multi_index_cursor(my_container &data) : data(data) {}
  bool inverse = false;
  int index = 0;
  using const_iterator = typename my_container::const_iterator;
  using const_iterator1 = typename my_container::nth_index_const_iterator<0>::type;
  using const_iterator2 = typename my_container::nth_index_const_iterator<1>::type;

  const_iterator begin{data.begin()}, end{data.end()};
  const_iterator1 begin1 = data.get<0>().cbegin(), end1 = data.get<0>().cend();
  const_iterator2 begin2 = data.get<1>().cbegin(), end2 = data.get<1>().cend();

  sqlite::result<void> next()
  {
    switch (index)
    {
      case 0: inverse ? end--  : begin++;  break;
      case 1: inverse ? end1-- : begin1++; break;
      case 2: inverse ? end2-- : begin2++; break;
    }
    return {};
  }

  sqlite::result<sqlite3_int64> row_id()
  {
    static_assert(sizeof(const_iterator) <= sizeof(sqlite3_int64), "");

    switch (index)
    {
      case 0: return reinterpret_cast<sqlite3_int64>(inverse ? &*std::prev(end ) : &*begin);
      case 1: return reinterpret_cast<sqlite3_int64>(inverse ? &*std::prev(end1) : &*begin1);
      case 2: return reinterpret_cast<sqlite3_int64>(inverse ? &*std::prev(end2) : &*begin2);
    }

    return sqlite3_int64();
  }
  sqlite::result<sqlite::string_view> column(int i, bool /*nochange*/)
  {
    const boost_library * lib;
    switch (index)
    {
      case 0: lib = &(inverse ? *std::prev(end) : *begin);   break;
      case 1: lib = &(inverse ? *std::prev(end1) : *begin1); break;
      case 2: lib = &(inverse ? *std::prev(end2) : *begin2); break;
    }

    const auto & elem = *lib;
    if (i == 0)
      return elem.name;
    else
      return elem.version;
  }


  sqlite::result<void> filter(int idx, const char * idxStr, span<sqlite::value> values)
  {
    inverse = (idx == 0b1000) & idx;
    index = idx & 0b11;

    boost::optional<sqlite::cstring_ref> lower, upper, equal;
    int lower_op = 0, upper_op = 0;

    bool surely_empty = false;

    for (auto i = 0u; i < values.size(); i++)
    {
      auto txt = values[i].get_text();
      switch (idxStr[i])
      {
        case SQLITE_INDEX_CONSTRAINT_EQ:
          if (equal && (*equal != txt))
            // two different equal constraints do that to you.
            surely_empty = true;
          else
            equal.emplace(txt);
          break;
        case SQLITE_INDEX_CONSTRAINT_GT:
        case SQLITE_INDEX_CONSTRAINT_GE:
          if (lower == txt)
          {
            // pick the more restrictive one
            if (lower_op == SQLITE_INDEX_CONSTRAINT_GE)
              lower_op = idxStr[i];
          }
          else
          {
            lower = (std::max)(lower.value_or(txt), txt);
            lower_op = idxStr[i];
          }

          break;
        case SQLITE_INDEX_CONSTRAINT_LE:
        case SQLITE_INDEX_CONSTRAINT_LT:
          if (upper == txt)
          {
            if (upper_op == SQLITE_INDEX_CONSTRAINT_LT)
              upper_op = idxStr[i];
          }
          else
          {
            upper = (std::min)(upper.value_or(txt), txt);
            upper_op = idxStr[i];
          }
          break;
      }
    }

    if (lower && equal && lower > equal)
      surely_empty = true;

    if (upper && equal && upper < equal)
      surely_empty = true;
    if (surely_empty)
    {
      end = begin;
      end1 = begin1;
      end2 = begin2;
      return {};
    }

    switch (index)
    {
      default: break;
      case 1:
        if (equal)
          std::tie(begin1, end1) = data.get<0>().equal_range(*equal);
        else
        {
          if (lower)
          {
            if (lower_op == SQLITE_INDEX_CONSTRAINT_GE)
              begin1 = data.get<0>().lower_bound(*lower);
            else // SQLITE_INDEX_CONSTRAINT_GT
              begin1 = data.get<0>().upper_bound(*lower);
          }
          if (upper)
          {
            if (upper_op == SQLITE_INDEX_CONSTRAINT_LE)
              end1 = data.get<0>().upper_bound(*upper);
            else // SQLITE_INDEX_CONSTRAINT_LT
              end1 = data.get<0>().lower_bound(*upper);
          }
          break;
          case 2:
            if (equal)
              std::tie(begin2, end2) = data.get<1>().equal_range(*equal);
            else
            {
              if (lower)
              {
                if (lower_op == SQLITE_INDEX_CONSTRAINT_GE)
                  begin2 = data.get<1>().lower_bound(*lower);
                else // SQLITE_INDEX_CONSTRAINT_GT
                  begin2 = data.get<1>().upper_bound(*lower);
              }

              if (upper)
              {
                if (upper_op == SQLITE_INDEX_CONSTRAINT_LE)
                  end2 = data.get<1>().upper_bound(*upper);
                else // SQLITE_INDEX_CONSTRAINT_LT
                  end2 = data.get<1>().lower_bound(*upper);
              }
            }
          break;
        }
    }
    return {};
  }

  bool eof() noexcept
  {
    switch (index)
    {
      case 0: return begin == end;
      case 1: return begin1 == end1;
      case 2: return begin2 == end2;
      default: return true;
    }
  }
};

struct map_impl final
    : sqlite::vtab::table<multi_index_cursor>,
      sqlite::vtab::modifiable
{
  my_container data;
  const char * declaration() override
  {
    return R"(
          create table url(
              name text primary key unique not null,
              version text);)";
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

  sqlite::result<cursor_type> open() override
  {
    return cursor_type(data);
  }

  sqlite::result<void> delete_(sqlite::value key) override
  {
    data.erase(key.get_text());
    return {};
  }
  sqlite::result<sqlite_int64> insert(sqlite::value /*key*/, span<sqlite::value> values,
                                      int /*on_conflict*/) override
  {
    data.insert({values[0].get_text(), values[1].get_text()});
    return 0;
  }

  sqlite::result<sqlite_int64> update(sqlite::value old_key, sqlite::value new_key,
                                      span<sqlite::value> values, int /*on_conflict*/) override
  {
    if (new_key.get_int() != old_key.get_int())
    {

      auto node = reinterpret_cast<my_container::value_type *>(old_key.get_int64());
      data.erase(data.iterator_to(*node));
    }

    auto res = data.insert({values[0].get_text(), values[1].get_text()});
    if (!res.second)
      data.replace(res.first, {values[0].get_text(), values[1].get_text()});
    return 0;
  }

  sqlite::result<void> best_index(sqlite::vtab::index_info & info) override
  {
    // we're using the index to encode the mode, because it's simple enough.
    // more complex application should use it as an index like intended

    int idx = 0;
    int idx_res = 0;
    sqlite::unique_ptr<char[]> str;
    // idx = 1 => name
    // idx = 2 => version
    if (!info.constraints().empty())
    {
      auto sz =  info.constraints().size() + 1u;
      str.reset(new (sqlite::memory_tag{}) char[sz]);
      std::memset(str.get(), '\0', sz);
    }

    for (auto & ct : info.constraints())
    {
      if (idx_res == 0) // if we're first, set the thing
        idx_res = (ct.iColumn + 1);
      // check if we're already building an index
      if (idx_res != (ct.iColumn + 1)) // wrong column, ignore.
        continue;
      if ( ct.usable != 0 ) // aye, that's us
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
            info.usage_of(ct).argvIndex = ++idx; // use it -> value in this position in `filter`.
            info.usage_of(ct).omit = 1; // tell sqlite that we're sure enough, so sqlite doesn't check
            break;
          default:
            break;
        }
      }
    }

    if (info.order_by().size() == 1u)
    {
      if ((info.order_by()[0].iColumn == 0)
          || (idx == 0) || (idx == 1))
      {
        info.set_already_ordered();
        if (info.order_by()[0].desc != 0)
          idx |= 0b1001; // encode inversion, because why not ?
      }
      else if ((info.order_by()[0].iColumn == 0) || (idx == 0) || (idx == 2))
      {
        info.set_already_ordered();
        if (info.order_by()[0].desc)
          idx |= 0b1010; // encode inversion, because why not ?
      }
    }
    info.set_index(idx_res);
    if (str)
      info.set_index_string(str.release(), true);
    return {};
  }
};

struct multi_index_map final : sqlite::vtab::eponymous_module<map_impl>
{
  sqlite::result<map_impl> connect(sqlite::connection, int, const char * const *)
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

void print(std::ostream & os, sqlite::resultset rw, boost::source_location loc = BOOST_CURRENT_LOCATION)
{
  os << loc.file_name() << "(" << loc.line() << "): ";
  os << "[";
  for (auto & r : rw)
    os << r.at(0).get_text() << ", ";
  os << "]" << std::endl;
}

int main (int /*argc*/, char * /*argv*/[])
{
  sqlite::connection conn{":memory:"};
  auto & m = sqlite::create_module(conn, "my_map", multi_index_map());
  boost::ignore_unused(m);

  {
    auto p = conn.prepare("insert into my_map (name, version) values (?, ?);");
    for (const auto & d : ::data)
      p.execute(d);
  }

  print(std::cout, conn.query("select * from my_map order by name desc;"));
  print(std::cout, conn.query("select * from my_map where name = 'url';"));
  print(std::cout, conn.query("select * from my_map where name <  'url';"));
  print(std::cout, conn.query("select * from my_map where name >= 'system' ;"));
  print(std::cout, conn.query("select * from my_map where name >= 'system' and name < 'url' ;"));
  print(std::cout, conn.query("select * from my_map where name > 'system' and name <= 'url' ;"));
  print(std::cout, conn.query("select * from my_map where name >  'json';"));
  print(std::cout, conn.query("select * from my_map where name >= 'json';"));
  print(std::cout, conn.query("select * from my_map where name <  'json';"));

  print(std::cout, conn.query("select * from my_map where name == 'json' order by name  asc;"));
  print(std::cout, conn.query("select * from my_map where name == 'json' and name == 'url';"));
  print(std::cout, conn.query("select * from my_map where name == 'json' order by name desc;"));

  print(std::cout, conn.query("select * from my_map where name < 'url' and name >= 'system' order by name desc;"));
  print(std::cout, conn.query("select * from my_map where version == '1.81.0';"));

  print(std::cout, conn.query("select * from my_map where version > '1.32.0' order by version desc;"));
  conn.query("delete from my_map where version == '1.81.0';");
  print(std::cout, conn.query("select * from my_map where name < 'system' and name <= 'system' ;"));


  return 0;
}