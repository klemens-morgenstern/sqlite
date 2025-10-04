//
// Copyright (c) 2025 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_QUERY_HPP
#define BOOST_SQLITE_QUERY_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/connection_ref.hpp>
#include <boost/sqlite/iterator.hpp>
#include <boost/sqlite/row.hpp>
#include <boost/sqlite/statement.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE

template<typename T = row>
struct query_range
{
  using iterator_type = sqlite::statement_iterator<T>;

  iterator_type begin() {return {stmt_};}
  iterator_type   end() {return {};}

  query_range(statement stmt) : stmt_(std::move(stmt)) {}

 private:
  statement stmt_;
};

template<typename T = row>
query_range<T> query(connection_ref conn, core::string_view q) { return {conn.prepare(q)};}

template<typename T = row>
query_range<T> query(connection_ref conn, core::string_view q, system::error_code &ec, error_info & ei)
{
  return {conn.prepare(q, ec, ei)};
}

template<typename T = row, typename ArgRange = std::initializer_list<param_ref>>
query_range<T> query(connection_ref conn, core::string_view q, ArgRange params)
{
    auto s = conn.prepare(q);
    s.bind(std::forward<ArgRange>(params));
    return {std::move(s)};
}

template<typename T = row, typename ArgRange = std::initializer_list<param_ref>>
query_range<T> query(connection_ref conn, core::string_view q, ArgRange params, system::error_code &ec, error_info & ei)
{
    auto s = conn.prepare(q, ec, ei);
    if (!ec)
      s.bind(std::forward<ArgRange>(params));
    return {std::move(s)};
}

template<typename T = row>
query_range<T> query(connection_ref conn, core::string_view q, std::initializer_list<std::pair<string_view, param_ref>> params)
{
    auto s = conn.prepare(q);
    s.bind(params);
    return {std::move(s)};
}

template<typename T = row>
query_range<T> query(connection_ref conn, core::string_view q, std::initializer_list<std::pair<string_view, param_ref>> params, system::error_code &ec, error_info & ei)
{
    auto s = conn.prepare(q, ec, ei);
    if (!ec)
      s.bind(params);
    return {std::move(s)};
}




BOOST_SQLITE_END_NAMESPACE

#endif // BOOST_SQLITE_QUERY_HPP
