//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_COLLATION_HPP
#define BOOST_SQLITE_COLLATION_HPP

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
        conn.native_handle(),
        name.c_str(),
        SQLITE_UTF8,
        static_cast<void*>(new func_type(std::forward<Func>(func))),
        +[](void * data, int len_l, const void * str_l, int len_r, const void * str_r) -> int
        {
          string_view l(static_cast<const char*>(str_l), len_l);
          string_view r(static_cast<const char*>(str_r), len_r);
          return (*static_cast<func_type*>(data))(l, r);
        },
        +[](void * p) { delete static_cast<func_type*>(p); }
      );
    if (res != SQLITE_OK)
      BOOST_SQLITE_ASSIGN_EC(ec, res);
}

template<typename Func>
auto create_collation(
    connection & conn,
    const std::string & name,
    Func && func)
    -> typename std::enable_if<
          std::is_convertible<
              decltype(func(string_view(), string_view())),
              int>::value>::type
{
    system::error_code ec;
    create_collation(conn, name, std::forward<Func>(func), ec);
    if (ec)
        throw_exception(system::system_error(ec));
}


}
}

#endif //BOOST_SQLITE_COLLATION_HPP
