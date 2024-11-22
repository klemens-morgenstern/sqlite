//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/json.hpp>
#include <boost/sqlite/resultset.hpp>

#include <boost/json/parse.hpp>
#include <boost/json/serializer.hpp>
#include <boost/json/value.hpp>
#include <boost/json/value_from.hpp>


BOOST_SQLITE_BEGIN_NAMESPACE


void tag_invoke(const struct set_result_tag &, sqlite3_context * ctx, const json::value & value)
{
  json::serializer ser;
  ser.reset(&value);

  sqlite3_int64 len = 4096;
  auto c = static_cast<char*>(sqlite3_malloc64(len));
  auto v = ser.read(c, len);

  while (!ser.done())
  {
    c = static_cast<char*>(sqlite3_realloc(c, len * 2));
    v = ser.read(c + len, len);
    len *= 2;
  }

  sqlite3_result_text(ctx, c, v.size(), sqlite3_free);
  sqlite3_result_subtype(ctx, json_subtype);
}


json::value as_json(const value & v, json::storage_ptr ptr)
{
  return json::parse(v.get_text(), ptr);
}

json::value as_json(const field & f, json::storage_ptr ptr)
{
  return json::parse(f.get_text(), ptr);
}


void tag_invoke( const json::value_from_tag &, json::value& val, const value & f)
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

void tag_invoke( const json::value_from_tag &, json::value& val, const field & f)
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

void tag_invoke( const json::value_from_tag &, json::value& val, resultset && rs)
{
  auto & obj = val.emplace_array();

  for (auto r : rs)
  {
    auto & row = obj.emplace_back(json::object(obj.storage())).get_object();
    for (auto c : r)
      row[c.column_name()] =  json::value_from(c, row.storage());
  }
}

BOOST_SQLITE_END_NAMESPACE

