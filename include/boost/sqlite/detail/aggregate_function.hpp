// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_DETAIL_AGGREGATE_FUNCTION_HPP
#define BOOST_SQLITE_DETAIL_AGGREGATE_FUNCTION_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/detail/catch.hpp>
#include <boost/sqlite/error.hpp>
#include <boost/sqlite/cstring_ref.hpp>
#include <boost/sqlite/memory.hpp>
#include <boost/sqlite/result.hpp>
#include <boost/sqlite/value.hpp>

#include <boost/callable_traits/args.hpp>
#include <boost/callable_traits/return_type.hpp>
#include <boost/callable_traits/has_void_return.hpp>
#include <boost/core/span.hpp>


BOOST_SQLITE_BEGIN_NAMESPACE

namespace detail
{

template<typename Func>
struct aggregate_function_maker
{
  void * mem;

  template<typename ... Args>
  Func* operator()(Args && ... args)
  {
    return new (mem) Func(std::forward<Args>(args)...);
  }
};

template<typename Func, typename Args>
int create_aggregate_function(sqlite3 * db, cstring_ref name, Args && args, int flags,
                              std::true_type /* void return */)
{
  using args_type    = callable_traits::args_t<decltype(&Func::step)>;
  using span_type    = typename std::tuple_element<1U, args_type>::type;
  using func_type    = typename std::decay<Func>::type;
  using func_args_type    = typename std::decay<Args>::type;

  return sqlite3_create_function_v2(
      db, name.c_str(),
      span_type::extent == boost::dynamic_extent ? -1 : static_cast<int>(span_type::extent),
      SQLITE_UTF8 | flags,
      new (memory_tag{}) func_args_type(std::forward<Args>(args)),
      nullptr,
      +[](sqlite3_context* ctx, int len, sqlite3_value** args)
      {
        auto aa =  reinterpret_cast<value*>(args);
        auto fa = reinterpret_cast<func_args_type*>(sqlite3_user_data(ctx));
        auto c = static_cast<func_type*>(sqlite3_aggregate_context(ctx, 0));

        execute_context_function(
            ctx,
            [&]() -> result<void>
            {
              if (c == nullptr)
              {
                auto p = sqlite3_aggregate_context(ctx, sizeof(func_type));
                if (!p)
                  return error(SQLITE_NOMEM);
                c = mp11::tuple_apply(aggregate_function_maker<func_type>{p}, *fa);
              }
              c->step(span_type{aa, static_cast<std::size_t>(len)});
              return {};
            });
      },
      [](sqlite3_context* ctx)
      {
        auto fa = reinterpret_cast<func_args_type*>(sqlite3_user_data(ctx));
        auto c = static_cast<func_type*>(sqlite3_aggregate_context(ctx, 0));

        execute_context_function(
            ctx,
            [&]() -> result<decltype(c->final())>
            {
              if (c == nullptr)
              {
                auto p = sqlite3_aggregate_context(ctx, sizeof(func_type));
                if (!p)
                  return error(SQLITE_NOMEM);
                c = mp11::tuple_apply(aggregate_function_maker<func_type>{p}, *fa);
              }
              struct reaper {void operator()(func_type * c) { c->~func_type();}};
              std::unique_ptr<func_type, reaper> cl{c};
              return c->final();
            });
      },
      [](void * ptr) noexcept { delete_(static_cast<func_type*>(ptr));}
  );
}


template<typename Func, typename Args>
int create_aggregate_function(sqlite3 * db, cstring_ref name, Args && args, int flags,
                              std::false_type /* void return */)
{
  using args_type    = callable_traits::args_t<decltype(&Func::step)>;
  using span_type    = typename std::tuple_element<1U, args_type>::type;
  using func_type    = typename std::decay<Func>::type;
  using func_args_type    = typename std::decay<Args>::type;

  return sqlite3_create_function_v2(
      db, name.c_str(),
      span_type::extent == boost::dynamic_extent ? -1 : static_cast<int>(span_type::extent),
      SQLITE_UTF8 | flags,
      new (memory_tag{}) func_args_type(std::forward<Args>(args)),
      nullptr,
      +[](sqlite3_context* ctx, int len, sqlite3_value** args)
      {
        auto aa =  reinterpret_cast<value*>(args);
        auto fa = reinterpret_cast<func_args_type*>(sqlite3_user_data(ctx));
        auto c = static_cast<func_type*>(sqlite3_aggregate_context(ctx, 0));

        execute_context_function(
            ctx,
            [&]() -> result<void>
            {
              if (c == nullptr)
              {
                auto p = sqlite3_aggregate_context(ctx, sizeof(func_type));
                if (!p)
                  return error(SQLITE_NOMEM);
                c = mp11::tuple_apply(aggregate_function_maker<func_type>{p}, *fa);
              }
              c->step(*c, span_type{aa, static_cast<std::size_t>(len)});
              return {};
            });
      },
      [](sqlite3_context* ctx)
      {
        auto fa = reinterpret_cast<func_args_type*>(sqlite3_user_data(ctx));
        auto c = static_cast<func_type*>(sqlite3_aggregate_context(ctx, 0));

        execute_context_function(
            ctx,
            [&]() -> result<decltype(c->final(*c))>
            {
              if (c == nullptr)
              {
                auto p = sqlite3_aggregate_context(ctx, sizeof(func_type));
                if (!p)
                  return error(SQLITE_NOMEM);
                c = mp11::tuple_apply(aggregate_function_maker<func_type>{p}, *fa);
              }
              struct reaper {void operator()(func_type * c) { c->~func_type();}};
              std::unique_ptr<func_type, reaper> cl{c};
              return c->final();
            });
      },
      [](void * ptr) noexcept { delete_(static_cast<func_type*>(ptr));}
  );
}

}

BOOST_SQLITE_END_NAMESPACE


#endif //BOOST_SQLITE_DETAIL_AGGREGATE_FUNCTION_HPP
