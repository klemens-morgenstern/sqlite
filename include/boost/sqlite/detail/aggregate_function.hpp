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
int create_aggregate_function(sqlite3 * db, cstring_ref name, Func && func,
                              std::true_type /* void return */)
{
  using args_type    = callable_traits::args_t<decltype(&Func::step)>;
  using context_type = typename std::remove_reference<typename std::tuple_element<1u, args_type>::type>::type;
  using span_type    = typename std::tuple_element<2U, args_type>::type;
  using func_type    = typename std::decay<Func>::type;

  return sqlite3_create_function_v2(
      db, name.c_str(),
      span_type::extent == boost::dynamic_extent ? -1 : static_cast<int>(span_type::extent),
      SQLITE_UTF8,
      new (memory_tag{}) func_type(std::forward<Func>(func)),
      nullptr,
      +[](sqlite3_context* ctx, int len, sqlite3_value** args)
      {
        auto aa =  reinterpret_cast<value*>(args);
        auto  f = reinterpret_cast<Func*>(sqlite3_user_data(ctx));
        auto c = static_cast<context_type*>(sqlite3_aggregate_context(ctx, 0));

        execute_context_function(
            ctx,
            [&]() -> result<void>
            {
              if (c == nullptr)
              {
                auto p = sqlite3_aggregate_context(ctx, sizeof(context_type));
                if (!p)
                  return error(SQLITE_NOMEM);
                c = new (p) context_type();
              }
              f->step(*c, span_type{aa, static_cast<std::size_t>(len)});
              return {};
            });
      },
      [](sqlite3_context* ctx)
      {
        auto  f = reinterpret_cast<Func*>(sqlite3_user_data(ctx));
        auto c = static_cast<context_type*>(sqlite3_aggregate_context(ctx, 0));

        execute_context_function(
            ctx,
            [&]() -> result<decltype(f->final(*c))>
            {
              if (c == nullptr)
              {
                auto p = sqlite3_aggregate_context(ctx, sizeof(context_type));
                if (!p)
                  return error(SQLITE_NOMEM);
                c = new (p) context_type();
              }
              struct reaper {void operator()(context_type * c) { c->~context_type();}};
              std::unique_ptr<context_type, reaper> cl{c};
              return f->final(*c);
            });
      },
      [](void * ptr) noexcept { delete_(static_cast<func_type*>(ptr));}
  );
}


template<typename Func>
int create_aggregate_function(sqlite3 * db, cstring_ref name, Func && func,
                              std::false_type /* void return */)
{
  using args_type    = callable_traits::args_t<decltype(&Func::step)>;
  using context_type = typename std::remove_reference<typename std::tuple_element<1u, args_type>::type>::type;
  using span_type    = typename std::tuple_element<2U, args_type>::type;
  using func_type    = typename std::decay<Func>::type;

  return sqlite3_create_function_v2(
      db, name.c_str(),
      span_type::extent == boost::dynamic_extent ? -1 : static_cast<int>(span_type::extent),
      SQLITE_UTF8,
      new (memory_tag{}) func_type(std::forward<Func>(func)),
      nullptr,
      +[](sqlite3_context* ctx, int len, sqlite3_value** args)
      {
        auto aa =  reinterpret_cast<value*>(args);
        auto  f = reinterpret_cast<Func*>(sqlite3_user_data(ctx));
        auto c = static_cast<context_type*>(sqlite3_aggregate_context(ctx, 0));

        execute_context_function(
            ctx,
            [&]() -> result<void>
            {
              if (c == nullptr)
              {
                auto p = sqlite3_aggregate_context(ctx, sizeof(context_type));
                if (!p)
                  return error(SQLITE_NOMEM);
                c = new (p) context_type();
              }
              f->step(*c, span_type{aa, static_cast<std::size_t>(len)});
              return {};
            });
      },
      [](sqlite3_context* ctx)
      {
        auto  f = reinterpret_cast<Func*>(sqlite3_user_data(ctx));
        auto c = static_cast<context_type*>(sqlite3_aggregate_context(ctx, 0));

        execute_context_function(
            ctx,
            [&]() -> result<decltype(f->final(*c))>
            {
              if (c == nullptr)
              {
                auto p = sqlite3_aggregate_context(ctx, sizeof(context_type));
                if (!p)
                  return error(SQLITE_NOMEM);
                c = new (p) context_type();
              }
              struct reaper {void operator()(context_type * c) { c->~context_type();}};
              std::unique_ptr<context_type, reaper> cl{c};
              return f->final(*c);
            });
      },
      [](void * ptr) noexcept { delete_(static_cast<func_type*>(ptr));}
  );
}

}

BOOST_SQLITE_END_NAMESPACE


#endif //BOOST_SQLITE_DETAIL_AGGREGATE_FUNCTION_HPP
