// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_IMPL_WINDOW_FUNCTION_HPP
#define BOOST_SQLITE_IMPL_WINDOW_FUNCTION_HPP
#if SQLITE_VERSION_NUMBER >= 3025000

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/cstring_ref.hpp>
#include <boost/sqlite/memory.hpp>
#include <boost/sqlite/result.hpp>
#include <boost/sqlite/value.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE

namespace detail
{

template<typename Func>
struct window_function_maker
{
  void * mem;

  template<typename ... Args>
  Func* operator()(Args && ... args)
  {
    return new (mem) Func(std::forward<Args>(args)...);
  }
};

template<typename Func, typename Args>
int create_window_function(sqlite3 * db, cstring_ref name, Args && args, int flags,
                           std::true_type /* is void */)
{
  using args_type    = callable_traits::args_t<decltype(&Func::step)>;
  using span_type    = typename std::tuple_element<1U, args_type>::type;
  using func_type    = typename std::decay<Func>::type;
  using func_args_type    = typename std::decay<Args>::type;

  return sqlite3_create_window_function(
      db, name.c_str(),
      span_type::extent == boost::dynamic_extent ? -1 : static_cast<int>(span_type::extent),
      SQLITE_UTF8 | flags,
      new (memory_tag{}) func_args_type(std::forward<Args>(args)),
      +[](sqlite3_context* ctx, int len, sqlite3_value** args) noexcept //xStep
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
                c = mp11::tuple_apply(window_function_maker<func_type>{p}, *fa);
              }
              c->step(span_type{aa, static_cast<std::size_t>(len)});
              return {};
            });

      },
      [](sqlite3_context* ctx) // xFinal
      {
        auto fa = reinterpret_cast<func_args_type*>(sqlite3_user_data(ctx));
        auto c = static_cast<func_type*>(sqlite3_aggregate_context(ctx, 0));

        execute_context_function(
            ctx,
            [&]() -> result<decltype(c->value())>
            {
              if (c == nullptr)
              {
                auto p = sqlite3_aggregate_context(ctx, sizeof(func_type));
                if (!p)
                  return error(SQLITE_NOMEM);
                c = mp11::tuple_apply(window_function_maker<func_type>{p}, *fa);
              }
              struct reaper {void operator()(func_type * c) { c->~func_type();}};
              std::unique_ptr<func_type, reaper> cl{c};

              return c->value();
            });
      },
      [](sqlite3_context* ctx) //xValue
      {
        auto fa = reinterpret_cast<func_args_type*>(sqlite3_user_data(ctx));
        auto c = static_cast<func_type*>(sqlite3_aggregate_context(ctx, 0));
        execute_context_function(
            ctx,
            [&]() -> result<decltype(c->value())>
            {
              if (c == nullptr)
              {
                auto p = sqlite3_aggregate_context(ctx, sizeof(func_type));
                if (!p)
                  return error(SQLITE_NOMEM);
                c = mp11::tuple_apply(window_function_maker<func_type>{p}, *fa);
              }
              return c->value();
            });

      },
      +[](sqlite3_context* ctx, int len, sqlite3_value** args) // xInverse
      {
        auto aa =  reinterpret_cast<value*>(args);
        auto fa = reinterpret_cast<func_args_type*>(sqlite3_user_data(ctx));
        auto c = static_cast<func_type*>(sqlite3_aggregate_context(ctx, 0));
        execute_context_function(
            ctx,
            [&]() -> result<decltype(c->inverse(span_type{aa, static_cast<std::size_t>(len)}))>
            {
              if (c == nullptr)
              {
                auto p = sqlite3_aggregate_context(ctx, sizeof(func_type));
                if (!p)
                  return error(SQLITE_NOMEM);
                c = mp11::tuple_apply(window_function_maker<func_type>{p}, *fa);
              }
              c->inverse(span_type{aa, static_cast<std::size_t>(len)});
              return {};
            });

      },
      [](void * ptr) /* xDestroy */ { delete_(static_cast<func_type*>(ptr));}
  );
}

template<typename Func, typename Args>
int create_window_function(sqlite3 * db, cstring_ref name, Args && args, int flags,
                           std::false_type /* is void */)
{
  using args_type    = callable_traits::args_t<decltype(&Func::step)>;
  using span_type    = typename std::tuple_element<1U, args_type>::type;
  using func_type    = typename std::decay<Func>::type;
  using func_args_type    = typename std::decay<Args>::type;

  return sqlite3_create_window_function(
      db, name.c_str(),
      span_type::extent == boost::dynamic_extent ? -1 : static_cast<int>(span_type::extent),
      SQLITE_UTF8 | flags,
      new (memory_tag{}) func_args_type(std::forward<Args>(args)),
      +[](sqlite3_context* ctx, int len, sqlite3_value** args) noexcept //xStep
      {
        auto aa =  reinterpret_cast<value*>(args);
        auto fa = reinterpret_cast<func_args_type*>(sqlite3_user_data(ctx));
        auto  c = static_cast<func_type*>(sqlite3_aggregate_context(ctx, 0));

        execute_context_function(
            ctx,
            [&]() -> result<decltype(c->step(span_type{aa, static_cast<std::size_t>(len)}))>
            {
              if (c == nullptr)
              {
                auto p = sqlite3_aggregate_context(ctx, sizeof(func_type));
                if (!p)
                  return {system::in_place_error, error(SQLITE_NOMEM)};
                c = mp11::tuple_apply(window_function_maker<func_type>{p}, *fa);
              }
              return c->step(span_type{aa, static_cast<std::size_t>(len)});
            });

      },
      [](sqlite3_context* ctx) // xFinal
      {
        auto fa = reinterpret_cast<func_args_type*>(sqlite3_user_data(ctx));
        auto c = static_cast<func_type*>(sqlite3_aggregate_context(ctx, 0));
        execute_context_function(
            ctx,
            [&]() -> result<decltype(c->value())>
            {
              if (c == nullptr)
              {
                auto p = sqlite3_aggregate_context(ctx, sizeof(func_type));
                if (!p)
                  return {system::in_place_error, error(SQLITE_NOMEM)};
                c = mp11::tuple_apply(window_function_maker<func_type>{p}, *fa);
              }

              struct reaper {void operator()(func_type * c) { c->~func_type();}};
              std::unique_ptr<func_type, reaper> cl{c};
              return c->value();
            });
      },
      [](sqlite3_context* ctx) //xValue
      {
        auto fa = reinterpret_cast<func_args_type*>(sqlite3_user_data(ctx));
        auto c = static_cast<func_type*>(sqlite3_aggregate_context(ctx, 0));
        execute_context_function(
            ctx,
            [&]() -> result<decltype(c->value())>
            {
              if (c == nullptr)
              {
                auto p = sqlite3_aggregate_context(ctx, sizeof(func_type));
                if (!p)
                  return {system::in_place_error, error(SQLITE_NOMEM)};
                c = mp11::tuple_apply(window_function_maker<func_type>{p}, *fa);
              }
              return c->value();
            });

      },
      +[](sqlite3_context* ctx, int len, sqlite3_value** args) // xInverse
      {
        auto aa =  reinterpret_cast<value*>(args);
        auto fa = reinterpret_cast<func_args_type*>(sqlite3_user_data(ctx));
        auto c = static_cast<func_type*>(sqlite3_aggregate_context(ctx, 0));
        execute_context_function(
            ctx,
            [&]() -> result<decltype(c->inverse(span_type{aa, static_cast<std::size_t>(len)}))>
            {
              if (c == nullptr)
              {
                auto p = sqlite3_aggregate_context(ctx, sizeof(func_type));
                if (!p)
                  return {system::in_place_error, error(SQLITE_NOMEM)};
                c = mp11::tuple_apply(window_function_maker<func_type>{p}, *fa);
              }
              return c->inverse(span_type{aa, static_cast<std::size_t>(len)});
            });

      },
      [](void * ptr) /* xDestroy */ { delete_(static_cast<func_type*>(ptr));}
  );
}


}

BOOST_SQLITE_END_NAMESPACE
#endif // SQLITE_VERSION
#endif //BOOST_SQLITE_IMPL_WINDOW_FUNCTION_HPP
