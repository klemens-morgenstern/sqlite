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

// this examples shows how to expose an ordered map as a vtable.

// tag::cursor[]
struct ordered_map_cursor final : sqlite::vtab::cursor<sqlite::string_view> // <1>
{
  container::flat_map<std::string, std::string> &data;
  ordered_map_cursor(container::flat_map<std::string, std::string> &data) : data(data) {}
  bool inverse = false;

  using const_iterator = typename container::flat_map<std::string, std::string>::const_iterator;
  const_iterator begin{data.begin()}, end{data.end()}; // <2>

  sqlite::result<void> next() override { if (inverse) end--; else begin++; return {};} // <3>

  sqlite::result<sqlite3_int64> row_id() override
  {
    return {system::in_place_error, SQLITE_MISUSE, // <4>
            "this shouldn't be called, we're omitting the row id"};
  }
  sqlite::result<sqlite::string_view> column(int i, bool /*nochange*/) override // <5>
  {
    auto & elem = inverse ? *std::prev(end) : *begin;

    if (i == 0)
      return elem.first;
    else
      return elem.second;
  }
  // end::cursor[]
  //tag::filter[]
  sqlite::result<void> filter(int idx, const char * idxStr, span<sqlite::value> values) override
  {
    if (idx != 0) // <1>
      inverse = true;

    for (auto i = 0u; i < values.size(); i ++)
    {
      auto txt = values[i].get_text();
      switch (idxStr[i])
      {
        case SQLITE_INDEX_CONSTRAINT_EQ: // <2>
        {
          auto nw = data.equal_range(txt);
          if (nw.first > begin)
            begin = nw.first;
          if (nw.second < end)
            end = nw.second;
        }

          break;
        case SQLITE_INDEX_CONSTRAINT_GT: // <3>
        {
          auto new_begin = data.find(txt);
          new_begin ++;
          if (new_begin > begin)
            begin = new_begin;
        }
          break;
        case SQLITE_INDEX_CONSTRAINT_GE: // <3>
        {
          auto new_begin = data.find(txt);
          if (new_begin > begin)
            begin = new_begin;
        }
          break;
        case SQLITE_INDEX_CONSTRAINT_LE: // <4>
        {
          auto new_end = data.find(txt);
          new_end++;
          if (new_end < end)
            end = new_end;

        }
          break;
        case SQLITE_INDEX_CONSTRAINT_LT: // <4>
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
  //end::filter[]
  // tag::cursor[]
  bool eof() noexcept override // <6>
  {
    return begin == end;
  }
};
// end::cursor[]


// tag::table[]
struct map_impl final
    : sqlite::vtab::table<ordered_map_cursor>,
      sqlite::vtab::modifiable // <1>

{
  container::flat_map<std::string, std::string> &data;
  map_impl(container::flat_map<std::string, std::string> &data) : data(data) {}

  const char * declaration() override // <2>
  {
    return R"(
          create table my_map(
              name text primary key unique not null,
              data text) WITHOUT ROWID;)";
  }


  sqlite::result<cursor_type> open() override // <3>
  {
    return cursor_type{data};
  }

  sqlite::result<void> delete_(sqlite::value key) override // <4>
  {
    data.erase(key.get_text());
    return {};
  }
  sqlite::result<sqlite_int64> insert(sqlite::value /*key*/, span<sqlite::value> values,
                                      int /*on_conflict*/) override // <5>
  {
    data.emplace(values[0].get_text(), values[1].get_text());
    return 0;
  }

  sqlite::result<sqlite_int64> update(sqlite::value old_key, sqlite::value new_key,
                                      span<sqlite::value> values,
                                      int /*on_conflict*/) override // <6>
  {
    if (new_key.get_int() != old_key.get_int())
      data.erase(old_key.get_text());
    data.insert_or_assign(values[0].get_text(), values[1].get_text());
    return 0;
  }

  // end::table[]
  // tag::best_index[]
  sqlite::result<void> best_index(sqlite::vtab::index_info & info) override
  {
    // we're using the index to encode the mode, because it's simple enough.
    // more complex application should use it as an index like intended

    int idx = 0;
    sqlite::unique_ptr<char[]> str; // <1>
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
      if ((idx & SQLITE_INDEX_CONSTRAINT_EQ) != 0)
        break;
      auto ct = info.constraints()[i];
      if (ct.iColumn == 0
          && ct.usable != 0) // aye, that's us
      {
        switch (ct.op) //<2>
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
      idx |= info.order_by()[0].desc; // <3>
      info.set_already_ordered(); // <4>
    }

    // <5>
    info.set_index(idx);
    if (str)
      info.set_index_string(str.release(), true);

    return {};
  }
  // end::best_index[]
  // tag::table[]
};

// end::table[]

// tag::module[]
struct ordered_map_module final : sqlite::vtab::eponymous_module<map_impl>
{
  container::flat_map<std::string, std::string> data;
  template<typename ... Args>
  ordered_map_module(Args && ...args) : data(std::forward<Args>(args)...) {}

  sqlite::result<map_impl> connect(
      sqlite::connection_ref /*conn*/, int /*argc*/, const char * const */*argv*/)
  {
    return map_impl{data};
  }
};
// end::module[]



std::initializer_list<std::pair<std::string, std::string>> init_data = {
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

void print(std::ostream & os, sqlite::statement rw)
{
  os << "[";
  for (auto & r : sqlite::statement_range(rw))
    os << r.at(0).get_text() << ", ";
  os << "]" << std::endl;
}

int main (int /*argc*/, char * /*argv*/[])
{
  sqlite::connection conn{":memory:"};

  // tag::module[]
  ordered_map_module & m = sqlite::create_module(conn, "my_map", ordered_map_module(init_data));
  // end::module[]
  boost::ignore_unused(m);

  print(std::cout, conn.prepare("select * from my_map order by name desc;"));
  print(std::cout, conn.prepare("select * from my_map where name = 'url';"));
  print(std::cout, conn.prepare("select * from my_map where name < 'url' and name >= 'system' ;"));
  print(std::cout, conn.prepare("select * from my_map where name >  'json';"));
  print(std::cout, conn.prepare("select * from my_map where name >= 'json';"));
  print(std::cout, conn.prepare("select * from my_map where name <  'json';"));
  print(std::cout, conn.prepare("select * from my_map where name == 'json' order by name  asc;"));
  print(std::cout, conn.prepare("select * from my_map where name == 'json' order by name desc;"));

  print(std::cout, conn.prepare("select * from my_map where name < 'url' and name >= 'system' order by name desc;"));
  print(std::cout, conn.prepare("select * from my_map where data == '1.81.0';"));

  conn.prepare("delete from my_map where data == '1.81.0';");

  return 0;
}
