//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_COLLATION_HPP
#define BOOST_SQLITE_COLLATION_HPP

#include <boost/sqlite/connection.hpp>

namespace boost
{
namespace sqlite
{


template<typename Func>
void create_collation(
    connection & conn,
    const std::string & name,
    Func && func,
    typename std::enable_if<
        std::is_convertible<
            decltype(func(string_view(), string_view())),
            int>::value,
        system::error_code>::type & ec)
{
    using func_type = typename std::decay<Func>::type;
    auto res = sqlite3_create_collation_v2(
        conn.handle(),
        name.c_str(),
        SQLITE_UTF8,
        static_cast<void*>(new func_type(std::forward<Func>(func))),
        +[](void * data, int len_l, const void * str_l, int len_r, const void * str_r) -> int
        {
          string_view l(static_cast<const char*>(str_l), len_l);
          string_view r(static_cast<const char*>(str_r), len_r);
          auto & impl = (*static_cast<func_type*>(data));
          static_assert(noexcept(impl(l, r)),
                        "Collation function must be noexcept");
          return impl(l, r);
        },
        +[](void * p) { delete static_cast<func_type*>(p); }
      );
    if (res != SQLITE_OK)
      BOOST_SQLITE_ASSIGN_EC(ec, res);
}

/** @brief Define a custom collation function
 @ingroup reference

 @param conn A connection to the database in which to install the collation.
 @param name The name of the collation.
 @param func The function

 The function must be callable with two `string_view` and return an int, indicating the comparison results.

 @par Example

 @code{.cpp}

 // a case insensitive string omparison, e.g. from boost.urls
 int ci_compare(string_view s0, string_view s1) noexcept;

 extern sqlite::connection conn;

 // Register the collation
 sqlite::create_collation(conn, "iequal", &ci_compare);

 // use the collation to get by name, case insensitively
 conn.query("select first_name, last_name from people where first_name = 'Klemens' collate iequal;");

 // order by names case insensitively
 conn.query("select * from people order by last_name collate iequal asc;");

 @endcode

 */

template<typename Func>
auto create_collation(
    connection & conn,
    const std::string & name,
    Func && func)
#if !defined(BOOST_SQLITE_GENERATING_DOCS)
    -> typename std::enable_if<
          std::is_convertible<
              decltype(func(string_view(), string_view())),
              int>::value>::type
#endif
{
    system::error_code ec;
    create_collation(conn, name, std::forward<Func>(func), ec);
    if (ec)
        throw_exception(system::system_error(ec));
}


}
}

#endif //BOOST_SQLITE_COLLATION_HPP
