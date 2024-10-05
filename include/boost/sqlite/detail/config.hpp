//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_DETAIL_CONFIG_HPP
#define BOOST_SQLITE_DETAIL_CONFIG_HPP

#include <boost/config.hpp>
#include <boost/core/detail/string_view.hpp>

#if defined(BOOST_SQLITE_COMPILE_EXTENSION)
#include <sqlite3ext.h>
#define BOOST_SQLITE_COMPILING_EXTENSION 1
#define BOOST_SQLITE_BEGIN_NAMESPACE namespace boost { namespace sqlite {  inline namespace ext {
#define BOOST_SQLITE_END_NAMESPACE } } }
#else
#include <sqlite3.h>
#define BOOST_SQLITE_BEGIN_NAMESPACE namespace boost { namespace sqlite {
#define BOOST_SQLITE_END_NAMESPACE } }
#endif

// copied from boost.json
#if defined(BOOST_SQLITE_DOCS)
# define BOOST_SQLITE_DECL
#else
# if (defined(BOOST_SQLITE_DYN_LINK) || defined(BOOST_ALL_DYN_LINK)) && !defined(BOOST_SQLITE_STATIC_LINK)
#  if defined(BOOST_SQLITE_SOURCE)
#   define BOOST_SQLITE_DECL        BOOST_SYMBOL_EXPORT
#  else
#   define BOOST_SQLITE_DECL        BOOST_SYMBOL_IMPORT
#  endif
# endif // static lib
# ifndef  BOOST_SQLITE_DECL
#  define BOOST_SQLITE_DECL
# endif
# if !defined(BOOST_SQLITE_SOURCE) && !defined(BOOST_ALL_NO_LIB) && !defined(BOOST_SQLITE_NO_LIB)
#  define BOOST_LIB_NAME boost_sqlite
#  if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_SQLITE_DYN_LINK)
#   define BOOST_DYN_LINK
#  endif
#  include <boost/config/auto_link.hpp>
# endif
#endif

BOOST_SQLITE_BEGIN_NAMESPACE


#if defined(BOOST_SQLITE_COMPILE_EXTENSION)
extern const sqlite3_api_routines *sqlite3_api;
#endif

using string_view = boost::core::string_view;

BOOST_SQLITE_END_NAMESPACE

#define BOOST_SQLITE_RETURN_EC(ev)                                              \
{                                                                               \
  static constexpr auto loc##__LINE__((BOOST_CURRENT_LOCATION));                \
  return ::boost::system::error_code(ev, boost::sqlite::sqlite_category(), &loc##__LINE__);   \
}

#define BOOST_SQLITE_ASSIGN_EC(ec, ev)                              \
{                                                                   \
  static constexpr auto loc##__LINE__((BOOST_CURRENT_LOCATION));    \
  ec.assign(ev, boost::sqlite::sqlite_category(), &loc##__LINE__); \
}

#if defined(BOOST_SQLITE_NO_VIRTUAL)
#define BOOST_SQLITE_VIRTUAL
#define BOOST_SQLITE_PURE
#else
#define BOOST_SQLITE_VIRTUAL virtual
#define BOOST_SQLITE_PURE = 0
#endif

#endif // BOOST_SQLITE_DETAIL_HPP