//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_JSON_HPP
#define BOOST_SQLITE_JSON_HPP

#include <sqlite3.h>
#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/field.hpp>
#include <boost/sqlite/value.hpp>
#include <boost/json/fwd.hpp>
#include <boost/json/storage_ptr.hpp>

namespace boost
{
namespace sqlite
{

struct resultset;
struct field;
struct value;

constexpr int json_subtype = static_cast<int>('J');

namespace detail
{

BOOST_SQLITE_DECL void set_result(sqlite3_context * ctx, const json::value & value);

}

inline bool is_json(const value & v) { return v.subtype() == json_subtype; }
inline bool is_json(const field & f) { return f.get_value().subtype() == json_subtype; }

BOOST_SQLITE_DECL json::value as_json(const value & v, json::storage_ptr ptr = {});
BOOST_SQLITE_DECL json::value as_json(const field & f, json::storage_ptr ptr = {});
BOOST_SQLITE_DECL json::value as_json(resultset & rs, json::storage_ptr ptr = {});

BOOST_SQLITE_DECL void tag_invoke( const json::value_from_tag &, json::value& val, const value & f);
BOOST_SQLITE_DECL void tag_invoke( const json::value_from_tag &, json::value& val, const field & f);
BOOST_SQLITE_DECL void tag_invoke( const json::value_from_tag &, json::value& val, resultset && rs);


}
}


#endif //BOOST_SQLITE_JSON_HPP
