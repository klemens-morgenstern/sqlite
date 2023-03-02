// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_STRING_HPP
#define BOOST_SQLITE_STRING_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/cstring_ref.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE

inline
bool like(
    cstring_ref lhs,
    cstring_ref rhs,
    char escape = '\0')
{
  return sqlite3_strlike(lhs.c_str(), rhs.c_str(), escape) != 0;
}

inline
bool glob(
    cstring_ref lhs,
    cstring_ref rhs)
{
  return sqlite3_strglob(lhs.c_str(), rhs.c_str()) != 0;
}

inline
int icmp(
    cstring_ref lhs,
    cstring_ref rhs)
{
  return sqlite3_stricmp(lhs.c_str(), rhs.c_str());
}

inline
int icmp(
    core::string_view lhs,
    core::string_view rhs,
    std::size_t n)
{
  return sqlite3_strnicmp(lhs.data(), rhs.data(), static_cast<int>(n));
}

BOOST_SQLITE_END_NAMESPACE

#endif //BOOST_SQLITE_STRING_HPP
