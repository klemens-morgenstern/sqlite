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
#include <boost/sqlite/value.hpp>
#include <boost/json/fwd.hpp>
#include <boost/json/storage_ptr.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE


struct resultset;
struct field;
struct value;

/// The subtype value used by the sqlite json extension. See the [sqlite reference](https://www.sqlite.org/json1.html)
constexpr int json_subtype = static_cast<int>('J');

BOOST_SQLITE_DECL void tag_invoke(const struct set_result_tag &, sqlite3_context * ctx, const json::value & value);


///@{
/// Qbrief Check if the value or field is a json. @ingroup reference
inline bool is_json(const value & v) { return v.type() == value_type::text && v.subtype() == json_subtype; }
inline bool is_json(const field & f) { return f.type() == value_type::text && f.get_value().subtype() == json_subtype; }
///@}

///@{
/// Qbrief Convert the value or field to a json. @ingroup reference
BOOST_SQLITE_DECL json::value as_json(const value & v, json::storage_ptr ptr = {});
BOOST_SQLITE_DECL json::value as_json(const field & f, json::storage_ptr ptr = {});
///@}

BOOST_SQLITE_DECL void tag_invoke( const json::value_from_tag &, json::value& val, const value & f);
BOOST_SQLITE_DECL void tag_invoke( const json::value_from_tag &, json::value& val, const field & f);
BOOST_SQLITE_DECL void tag_invoke( const json::value_from_tag &, json::value& val, resultset && rs);

BOOST_SQLITE_END_NAMESPACE

#if defined(BOOST_SQLITE_HEADER_ONLY)
#include <boost/sqlite/impl/json.ipp>
#endif

#endif //BOOST_SQLITE_JSON_HPP
