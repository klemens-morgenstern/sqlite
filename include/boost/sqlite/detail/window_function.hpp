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
int create_window_function(sqlite3 * db, cstring_ref name, Func && func,
                           std::true_type /* is void */)
{
  using args_type    = callable_traits::args_t<decltype(&Func::step)>;
  using context_type = typename std::remove_reference<typename std::tuple_element<1u, args_type>::type>::type;
  using span_type    = typename std::tuple_element<2U, args_type>::type;
  using func_type    = typename std::decay<Func>::type;

  return sqlite3_create_window_function(
      db, name.c_str(),
      span_type::extent == boost::dynamic_extent ? -1 : static_cast<int>(span_type::extent),
      SQLITE_UTF8,
      new (memory_tag{}) func_type(std::forward<Func>(func)),
      +[](sqlite3_context* ctx, int len, sqlite3_value** args) noexcept //xStep
      {
        auto aa =  reinterpret_cast<value*>(args);
        auto  f = reinterpret_cast<Func*>(sqlite3_user_data(ctx));
        auto c = static_cast<context_type*>(sqlite3_aggregate_context(ctx, 0));

        execute_context_function(
            ctx,
            [&]() -> leaf::result<void>
            {
              if (c == nullptr)
              {
                auto p = sqlite3_aggregate_context(ctx, sizeof(context_type));
                if (!p)
                  return leaf::new_error(SQLITE_NOMEM);
                c = new (p) context_type();
              }
              f->step(*c, span_type{aa, static_cast<std::size_t>(len)});
              return {};
            });

      },
      [](sqlite3_context* ctx) // xFinal
      {
        auto  f = reinterpret_cast<Func*>(sqlite3_user_data(ctx));
        auto c = static_cast<context_type*>(sqlite3_aggregate_context(ctx, 0));
        execute_context_function(
            ctx,
            [&]() -> leaf::result<void>
            {
              if (c == nullptr)
              {
                auto p = sqlite3_aggregate_context(ctx, sizeof(context_type));
                if (!p)
                  return leaf::new_error(SQLITE_NOMEM);
                c = new (p) context_type();
              }
              struct reaper {void operator()(context_type * c) { c->~context_type();}};
              std::unique_ptr<context_type, reaper> cl{c};

              set_result(ctx, f->value(*c));
              return {};
            });
      },
      [](sqlite3_context* ctx) //xValue
      {
        auto  f = reinterpret_cast<Func*>(sqlite3_user_data(ctx));
        auto c = static_cast<context_type*>(sqlite3_aggregate_context(ctx, 0));
        execute_context_function(
            ctx,
            [&]() -> leaf::result<void>
            {
              if (c == nullptr)
              {
                auto p = sqlite3_aggregate_context(ctx, sizeof(context_type));
                if (!p)
                  return leaf::new_error(SQLITE_NOMEM);
                c = new (p) context_type();
              }
              set_result(ctx, f->value(*c));
              return {};
            });

      },
      +[](sqlite3_context* ctx, int len, sqlite3_value** args) // xInverse
      {
        auto aa =  reinterpret_cast<value*>(args);
        auto  f = reinterpret_cast<Func*>(sqlite3_user_data(ctx));
        auto c = static_cast<context_type*>(sqlite3_aggregate_context(ctx, 0));
        execute_context_function(
            ctx,
            [&]() -> leaf::result<void>
            {
              if (c == nullptr)
              {
                auto p = sqlite3_aggregate_context(ctx, sizeof(context_type));
                if (!p)
                  return leaf::new_error(SQLITE_NOMEM);
                c = new (p) context_type();
              }
              f->inverse(*c, span_type{aa, static_cast<std::size_t>(len)});
              return {};
            });

      },
      [](void * ptr) /* xDestroy */ { delete_(static_cast<func_type*>(ptr));}
  );
}

template<typename Func>
int create_window_function(sqlite3 * db, cstring_ref name, Func && func,
                           std::false_type /* is void */)
{
  using args_type    = callable_traits::args_t<decltype(&Func::step)>;
  using context_type = typename std::remove_reference<typename std::tuple_element<1u, args_type>::type>::type;
  using span_type    = typename std::tuple_element<2U, args_type>::type;
  using func_type    = typename std::decay<Func>::type;

  return sqlite3_create_window_function(
      db, name.c_str(),
      span_type::extent == boost::dynamic_extent ? -1 : static_cast<int>(span_type::extent),
      SQLITE_UTF8,
      new (memory_tag{}) func_type(std::forward<Func>(func)),
      +[](sqlite3_context* ctx, int len, sqlite3_value** args) noexcept //xStep
      {
        auto aa =  reinterpret_cast<value*>(args);
        auto  f = reinterpret_cast<Func*>(sqlite3_user_data(ctx));
        auto c = static_cast<context_type*>(sqlite3_aggregate_context(ctx, 0));

        execute_context_function(
            ctx,
            [&]() -> leaf::result<void>
            {
              if (c == nullptr)
              {
                auto p = sqlite3_aggregate_context(ctx, sizeof(context_type));
                if (!p)
                  return leaf::new_error(SQLITE_NOMEM);
                c = new (p) context_type();
              }
              return f->step(*c, span_type{aa, static_cast<std::size_t>(len)});
            });

      },
      [](sqlite3_context* ctx) // xFinal
      {
        auto  f = reinterpret_cast<Func*>(sqlite3_user_data(ctx));
        auto c = static_cast<context_type*>(sqlite3_aggregate_context(ctx, 0));
        execute_context_function(
            ctx,
            [&]() -> leaf::result<void>
            {
              if (c == nullptr)
              {
                auto p = sqlite3_aggregate_context(ctx, sizeof(context_type));
                if (!p)
                  return leaf::new_error(SQLITE_NOMEM);
                c = new (p) context_type();
              }

              struct reaper {void operator()(context_type * c) { c->~context_type();}};
              std::unique_ptr<context_type, reaper> cl{c};
              BOOST_LEAF_AUTO(val, f->value(*c));
              set_result(ctx, std::move(val));

              return val.error();
            });
      },
      [](sqlite3_context* ctx) //xValue
      {
        auto  f = reinterpret_cast<Func*>(sqlite3_user_data(ctx));
        auto c = static_cast<context_type*>(sqlite3_aggregate_context(ctx, 0));
        execute_context_function(
            ctx,
            [&]() -> leaf::result<void>
            {
              if (c == nullptr)
              {
                auto p = sqlite3_aggregate_context(ctx, sizeof(context_type));
                if (!p)
                  return leaf::new_error(SQLITE_NOMEM);
                c = new (p) context_type();
              }
              BOOST_LEAF_AUTO(val, f->value(*c));
              set_result(ctx, std::move(val));
              return {};
            });

      },
      +[](sqlite3_context* ctx, int len, sqlite3_value** args) // xInverse
      {
        auto aa =  reinterpret_cast<value*>(args);
        auto  f = reinterpret_cast<Func*>(sqlite3_user_data(ctx));
        auto c = static_cast<context_type*>(sqlite3_aggregate_context(ctx, 0));
        execute_context_function(
            ctx,
            [&]() -> leaf::result<void>
            {
              if (c == nullptr)
              {
                auto p = sqlite3_aggregate_context(ctx, sizeof(context_type));
                if (!p)
                  return leaf::new_error(SQLITE_NOMEM);
                c = new (p) context_type();
              }
              return f->inverse(*c, span_type{aa, static_cast<std::size_t>(len)});
            });

      },
      [](void * ptr) /* xDestroy */ { delete_(static_cast<func_type*>(ptr));}
  );
}


}

BOOST_SQLITE_END_NAMESPACE
#endif // SQLITE_VERSION
#endif //BOOST_SQLITE_IMPL_WINDOW_FUNCTION_HPP
