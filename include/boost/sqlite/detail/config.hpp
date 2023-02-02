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

namespace boost
{
namespace sqlite
{

using string_view = boost::core::string_view;

}
}

#ifndef BOOST_SQLITE_HEADER_ONLY
# ifndef BOOST_SQLITE_SEPARATE_COMPILATION
#   define BOOST_SQLITE_HEADER_ONLY 1
# endif
#endif

#if defined(BOOST_SQLITE_HEADER_ONLY)
# define BOOST_SQLITE_DECL inline
#else
# define BOOST_SQLITE_DECL
#endif

#define BOOST_SQLITE_RETURN_EC(ev)                                              \
{                                                                               \
  static constexpr auto loc##__LINE__((BOOST_CURRENT_LOCATION));                \
  return ::boost::system::error_code(static_cast<error>(ev), &loc##__LINE__);   \
}

#define BOOST_SQLITE_ASSIGN_EC(ec, ev)                            \
{                                                                 \
  static constexpr auto loc##__LINE__((BOOST_CURRENT_LOCATION));  \
  ec.assign(static_cast<error>(ev), &loc##__LINE__);              \
}

#endif // BOOST_SQLITE_DETAIL_HPP