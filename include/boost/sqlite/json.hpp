//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_JSON_HPP
#define BOOST_SQLITE_JSON_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/field.hpp>
#include <boost/sqlite/iterator.hpp>
#include <boost/sqlite/statement.hpp>
#include <boost/sqlite/value.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/serializer.hpp>
#include <boost/json/storage_ptr.hpp>
#include <boost/json/value_from.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE

struct field;
struct value;

/// @brief The subtype value used by the sqlite json extension. See the [sqlite reference](https://www.sqlite.org/json1.html)
constexpr int json_subtype = static_cast<int>('J');

inline void tag_invoke(const struct set_result_tag &, sqlite3_context * ctx, const json::value & value)
{
  json::serializer ser;
  ser.reset(&value);

  sqlite3_int64 len = 4096;
  unique_ptr<char> c{static_cast<char*>(sqlite3_malloc64(len))};

  len = sqlite3_msize(c.get());
  auto v = ser.read(c.get(), len);

  while (!ser.done())
  {
    auto l = v.size();
    len *= 2;
    c.reset(static_cast<char*>(sqlite3_realloc(c.release(), len)));
    v = ser.read(c.get() + l, len);
  }

  sqlite3_result_text(ctx, c.release(), v.size(), sqlite3_free);
  sqlite3_result_subtype(ctx, json_subtype);
}

///@{
/// @brief Check if the value or field is a json. @ingroup reference
inline bool is_json(const value & v) { return v.type() == value_type::text && v.subtype() == json_subtype; }
inline bool is_json(const field & f) { return f.type() == value_type::text && f.get_value().subtype() == json_subtype; }
///@}

///@{
/// @brief Convert the value or field to a json. @ingroup reference
inline json::value as_json(const value & v, json::storage_ptr ptr = {})
{
  return json::parse(v.get_text(), ptr);
}
inline json::value as_json(const field & f, json::storage_ptr ptr = {})
{
  return json::parse(f.get_text(), ptr);
}
///@}


inline void tag_invoke( const json::value_from_tag &, json::value& val, const value & f)
{
  switch (f.type())
  {
    case value_type::integer:
      val.emplace_int64() = f.get_int();
    break;
    case value_type::floating:
      val.emplace_double() = f.get_double();
    break;
    case value_type::text:
    {
      auto txt = f.get_text();
      if (f.subtype() == json_subtype)
        val = json::parse(txt, val.storage());
      else
        val.emplace_string() = txt;
    }
    break;
    case value_type::blob:
      throw_exception(std::invalid_argument("cannot convert blob to json"));
    case value_type::null:
      default:
        val.emplace_null();
  }
}

inline void tag_invoke( const json::value_from_tag &, json::value& val, const field & f)
{
  switch (f.type())
  {
    case value_type::integer:
      val.emplace_int64() = f.get_int();
    break;
    case value_type::floating:
      val.emplace_double() = f.get_double();
    break;
    case value_type::text:
    {
      auto txt = f.get_text();
      if (f.get_value().subtype() == json_subtype)
        val = json::parse(txt, val.storage());
      else
        val.emplace_string() = txt;
    }
    break;
    case value_type::blob:
      throw_exception(std::invalid_argument("cannot convert blob to json"));
    case value_type::null:
      default:
        val.emplace_null();
  }
}

inline void tag_invoke( const json::value_from_tag &, json::value& val, statement && rs)
{
  auto & obj = val.emplace_array();

  for (auto r : sqlite::statement_range<sqlite::row>(rs))
  {
    auto & row = obj.emplace_back(json::object(obj.storage())).get_object();
    for (auto c : r)
      row[c.column_name()] =  json::value_from(c, row.storage());
  }
}

BOOST_SQLITE_END_NAMESPACE


#endif //BOOST_SQLITE_JSON_HPP
