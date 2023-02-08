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

#include <iomanip>
#include <iostream>

using namespace boost;

// add more conversions here if you need more types in the described struct
void assign_value(int & res, sqlite::value val) { res = val.get_int();}
void assign_value(std::string & res, sqlite::value val) { res = val.get_text();}

template<typename T>
struct describe_table
{
  boost::unordered_map<sqlite3_int64, T> data;
  constexpr static std::size_t column_count = mp11::mp_size<describe::describe_members<T, describe::mod_any_access>>::value;

  struct table_type
  {
    boost::unordered_map<sqlite3_int64, T> &data;
    std::string decl;
    const char * declaration()
    {
      if (!decl.empty())
        return decl.c_str();
      mp11::mp_for_each<describe::describe_members<T, describe::mod_public>>(
          [&](auto elem)
          {
              if (decl.empty())
                decl += "create table x(";
              else
                decl += ", ";

              decl += elem.name;
          });
      decl += ");";
      return decl.c_str();
    }


    sqlite3_int64 last_index = 0;

    struct cursor
    {
      typename boost::unordered_map<sqlite3_int64, T>::const_iterator itr, end;

      void next() {itr++;}
      sqlite3_int64 row_id() {return itr->first;}

      void column(sqlite::context<> ctx, int i, bool /* no_change */)
      {
          mp11::mp_with_index<column_count>(
              i, [&](auto Idx)
              {
                  using type = mp11::mp_at_c<describe::describe_members<T, describe::mod_public>, Idx>;
                  ctx.set_result(itr->second.*type::pointer);
              });
      }

      bool eof() noexcept {return itr == end;}
    };

    cursor open()
    {
      return cursor{data.begin(), data.end()};
    }

    void delete_(sqlite::value key)
    {
      data.erase(key.get_int64());
    }
    sqlite_int64 insert(sqlite::value key, span<sqlite::value> values)
    {
      T res;
      sqlite_int64 id = key.is_null() ? last_index++ : key.get_int();
      auto vtr = values.begin();
      mp11::mp_for_each<describe::describe_members<T, describe::mod_public>>(
          [&](auto elem)
          {
            assign_value(res.*elem.pointer , *vtr);
            vtr++;
          });

      auto itr = data.emplace(id, std::move(res)).first;
      return itr->first;

    }
    sqlite_int64 update(sqlite::value old_key, sqlite::value new_key, span<sqlite::value> values)
    {
      if (new_key.get_int() != old_key.get_int())
        data.erase(old_key.get_int64());
      auto & res = data[new_key.get_int64()];

      auto vtr = values.begin();
      mp11::mp_for_each<describe::describe_members<T, describe::mod_public>>(
          [&](auto elem)
          {
            assign_value(res.*elem.pointer , *vtr);
            vtr++;
          });
      return 0u;
    }
  };

  table_type connect(int argc, const char * const  argv[])
  {
    return table_type{data};
  }
};


void print_table(std::ostream & str, sqlite::resultset res)
{
  for (auto i = 0u; i < res.column_count(); i ++)
    str << "| " << std::setfill(' ') << std::setw(15) << res.column_name(i) << " ";

  str << "|\n";

  for (auto i = 0u; i < res.column_count(); i ++)
    str << "|-----------------";
  str << "|\n";

  for (auto && r : res)
  {
    for (auto i = 0u; i < res.column_count(); i ++)
      str << "| " << std::setfill(' ') << std::setw(15) << r.at(i).get_text() << " ";

    str << "|\n" ;
  }
  str << std::endl;
}


struct boost_library
{
  std::string name;
  int first_released;
  int standard;
};

BOOST_DESCRIBE_STRUCT(boost_library, (), (name, first_released, standard));


int main (int argc, char * argv[])
{
  sqlite::connection conn{":memory:"};
  auto & md = sqlite::create_module(conn, "boost_libraries", describe_table<boost_library>());

  {
    auto p = conn.prepare("insert into boost_libraries (name, first_released, standard) values ($name, $version, $std);");
    p.execute({{"name", "process"},         {"version", 64}, {"std", 11}});
    p.execute({{"name", "asio"},            {"version", 35}, {"std", 98}});
    p.execute({{"name", "bimap"},           {"version", 35}, {"std", 98}});
    p.execute({{"name", "circular_buffer"}, {"version", 35}, {"std", 98}});
    p.execute({{"name", "mpi"},             {"version", 35}, {"std", 98}});
    p.execute({{"name", "beast"},           {"version", 66}, {"std", 11}});
    p.execute({{"name", "describe"},        {"version", 77}, {"std", 14}});
  }
  print_table(std::cout, conn.query("select * from boost_libraries;"));
  conn.execute("update boost_libraries set standard = 11 where standard = 98;");
  print_table(std::cout, conn.query("select * from boost_libraries;"));

  conn.prepare("delete from boost_libraries where name = ?").execute({"mpi"});
  print_table(std::cout, conn.query("select * from boost_libraries;"));

  return 0;
}
