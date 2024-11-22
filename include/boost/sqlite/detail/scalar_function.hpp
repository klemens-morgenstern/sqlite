// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_DETAIL_SCALAR_FUNCTION_HPP
#define BOOST_SQLITE_DETAIL_SCALAR_FUNCTION_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/cstring_ref.hpp>
#include <boost/sqlite/memory.hpp>
#include <boost/sqlite/result.hpp>
#include <boost/sqlite/value.hpp>

#include <boost/callable_traits/args.hpp>
#include <boost/callable_traits/return_type.hpp>
#include <boost/callable_traits/has_void_return.hpp>
#include <boost/core/span.hpp>


BOOST_SQLITE_BEGIN_NAMESPACE

template<typename ... Args>
struct context;

namespace detail
{

template<typename Func, typename ... Args, std::size_t Extent>
auto create_scalar_function_impl(sqlite3 * db,
                                 cstring_ref name,
                                 Func && func,
                                 int flags,
                                 std::tuple<context<Args...>, boost::span<value, Extent>> * ,
                                 std::false_type /* void return */,
                                 std::false_type /* is pointer */) -> int
{
  using func_type = typename std::decay<Func>::type;
  return sqlite3_create_function_v2(
      db, name.c_str(),
      Extent == boost::dynamic_extent ? -1 : static_cast<int>(Extent),
      SQLITE_UTF8 | flags,
      new (memory_tag{}) func_type(std::forward<Func>(func)),
      +[](sqlite3_context* ctx, int len, sqlite3_value** args)
      {
        auto cc = context<Args...>(ctx);
        auto aa =  reinterpret_cast<value*>(args);
        auto &f = *reinterpret_cast<func_type*>(sqlite3_user_data(ctx));
        boost::span<value, Extent> vals{aa, static_cast<std::size_t>(len)};

        execute_context_function(ctx, f, cc, vals);
      }, nullptr, nullptr,
      +[](void * ptr) noexcept {delete_(static_cast<func_type*>(ptr));}
  );
}

template<typename Func, typename ... Args, std::size_t Extent>
auto create_scalar_function_impl(sqlite3 * db,
                                 cstring_ref name,
                                 Func && func, int flags,
                                 std::tuple<context<Args...>, boost::span<value, Extent>> * ,
                                 std::true_type /* void return */,
                                 std::false_type /* is pointer */) -> int
{
  using func_type = typename std::decay<Func>::type;
  return sqlite3_create_function_v2(
      db,
      name.c_str(),
      (Extent == boost::dynamic_extent) ? -1 : static_cast<int>(Extent),
      SQLITE_UTF8 | flags,
      new (memory_tag{}) func_type(std::forward<Func>(func)),
      +[](sqlite3_context* ctx, int len, sqlite3_value** args)
      {
        auto cc = context<Args...>(ctx);
        auto aa =  reinterpret_cast<value*>(args);
        auto &f = *reinterpret_cast<func_type*>(sqlite3_user_data(ctx));
        boost::span<value, Extent> vals{aa, static_cast<std::size_t>(len)};

        execute_context_function(
            ctx,
            [&]()
            {
              f(cc, vals);
              return result<void>();
            });

      }, nullptr, nullptr,
      +[](void * ptr){delete_(static_cast<func_type*>(ptr));}
  );
}


template<typename Func, typename ... Args, std::size_t Extent>
auto create_scalar_function_impl(sqlite3 * db,
                                 cstring_ref name,
                                 Func && func, int flags,
                                 std::tuple<context<Args...>, boost::span<value, Extent>> * ,
                                 std::false_type /* void return */,
                                 std::true_type /* is pointer */) -> int
{
  return sqlite3_create_function_v2(
      db, name.c_str(),
      Extent == boost::dynamic_extent ? -1 : static_cast<int>(Extent),
      SQLITE_UTF8 | flags,
      reinterpret_cast<void*>(func),
      +[](sqlite3_context* ctx, int len, sqlite3_value** args)
      {
        auto cc = context<Args...>(ctx);
        auto aa =  reinterpret_cast<value*>(args);
        auto &f = *reinterpret_cast<Func*>(sqlite3_user_data(ctx));

        boost::span<value, Extent> vals{aa, static_cast<std::size_t>(len)};

        execute_context_function(
            ctx, f, cc, vals);
      }, nullptr, nullptr, nullptr);
}

template<typename Func, typename ... Args, std::size_t Extent>
auto create_scalar_function_impl(sqlite3 * db,
                                 cstring_ref name,
                                 Func && func, int flags,
                                 std::tuple<context<Args...>, boost::span<value, Extent>> * ,
                                 std::true_type /* void return */,
                                 std::true_type /* is pointer */) -> int
{
  return sqlite3_create_function_v2(
      db,
      name.c_str(),
      (Extent == boost::dynamic_extent) ? -1 : static_cast<int>(Extent),
      SQLITE_UTF8 | flags,
      reinterpret_cast<void*>(func),
      +[](sqlite3_context* ctx, int len, sqlite3_value** args)
      {
        auto cc = context<Args...>(ctx);
        auto aa =  reinterpret_cast<value*>(args);
        auto &f = *reinterpret_cast<Func*>(sqlite3_user_data(ctx));
        boost::span<value, Extent> vals{aa, static_cast<std::size_t>(len)};
        execute_context_function(
            ctx,
            [&]()
            {
              f(cc, vals);
              return result<void>();
            });

      }, nullptr, nullptr, nullptr);
}

template<typename Func>
auto create_scalar_function(sqlite3 * db,
                     cstring_ref name,
                     Func && func,
                     int flags)
                     -> decltype(create_scalar_function_impl(
                         db, name, std::forward<Func>(func), flags,
                         static_cast<callable_traits::args_t<Func>*>(nullptr),
                         callable_traits::has_void_return<Func>{},
                         std::is_pointer<typename std::decay<Func>::type>{}
                         ))
{
  return create_scalar_function_impl(db, name, std::forward<Func>(func), flags,
                                     static_cast<callable_traits::args_t<Func>*>(nullptr),
                                     callable_traits::has_void_return<Func>{},
                                     std::is_pointer<typename std::decay<Func>::type>{});
}


}

BOOST_SQLITE_END_NAMESPACE

#endif //BOOST_SQLITE_DETAIL_SCALAR_FUNCTION_HPP
