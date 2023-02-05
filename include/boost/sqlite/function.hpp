//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_FUNCTION_HPP
#define BOOST_SQLITE_FUNCTION_HPP

#include <boost/core/span.hpp>
#include <boost/sqlite/blob.hpp>
#include <boost/sqlite/connection.hpp>
#include <boost/sqlite/detail/catch.hpp>
#include <boost/sqlite/result.hpp>
#include <boost/sqlite/value.hpp>
#include <boost/callable_traits/args.hpp>
#include <boost/callable_traits/has_void_return.hpp>
#include <boost/callable_traits/return_type.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE


/** @brief A context that can be passed into scalar functions.
  @ingroup reference

  @tparam Args The argument that can be stored in the context.

  @code{.cpp}
  extern sqlite::connection conn;

  sqlite::create_function(
    conn, "my_sum,,
    [](sqlite::context<std::size_t> ctx,
       boost::span<sqlite::value, 1u> args) -> std::size_t
    {
        auto value = args[0].get_int64();
        auto p = ctx.get_if<0>();
        if (p != nullptr) // increment the counter
            return (*p) += value;
        else // set the initial value
            ctx.set<0>(value);
        return value;
    });
  @endcode

*/
template<typename ... Args>
struct context
{
  template<std::size_t N>
  using element = mp11::mp_take_c<mp11::mp_list<Args...>, N>;

  /// Set the value in the context at position `Idx`
  template<std::size_t Idx>
  void set(element<Idx> value)
  {
    sqlite3_set_auxdata(ctx_, Idx, *static_cast<void**>(&value),
                        new element<Idx>(std::move(value)),
                        +[](void * ptr)
                        {
                          delete static_cast<element<Idx> *>(ptr);
                        });
  }

  /// Get the value in the context at position `Idx`. Throws if the value isn't set.
  template<std::size_t Idx>
  auto get() -> element<Idx>  &
  {
    using type = element<Idx> ;
    auto p = static_cast<type*>(sqlite3_get_auxdata(ctx_, Idx));
    if (p == nullptr)
      throw_exception(std::invalid_argument("argument not set"));
    return p;
  }

  /// Get the value in the context at position `Idx`. Returns nullptr .value isn't set.
  template<std::size_t Idx>
  auto get_if() -> element<Idx>  *
  {
    using type = element<Idx> ;
    return static_cast<type*>(sqlite3_get_auxdata(ctx_, Idx));
  }


  explicit context(sqlite3_context * ctx) noexcept : ctx_(ctx) {}

  /// Set the result through the context, instead of returning it.
  template<typename T>
  auto set_result(T && val)
#if !defined(BOOST_SQLITE_GENERATING_DOCS)
    -> decltype(detail::set_result(static_cast<sqlite3_context*>(nullptr), std::forward<T>(val)))
#endif
  {
    detail::set_result(ctx_, std::forward<T>(val));
  }

  /// Set the an error through the context, instead of returning it.
  auto set_error(const char * message, int code = SQLITE_ERROR)
  {
    sqlite3_result_error(ctx_, message, code);
  }
 private:
  sqlite3_context * ctx_;
};

namespace detail
{


template<typename Func, typename ... Args, std::size_t Extent>
auto create_scalar_function_impl(sqlite3 * db,
                          const std::string & name,
                          Func && func,
                          std::tuple<context<Args...>, boost::span<value, Extent>> * ,
                          std::false_type /* void return */,
                          std::false_type /* is pointer */) -> int
{
  using func_type = typename std::decay<Func>::type;
  return sqlite3_create_function_v2(
      db, name.c_str(),
      Extent == boost::dynamic_extent ? -1 : static_cast<int>(Extent),
      SQLITE_UTF8,
      new func_type(std::forward<Func>(func)),
      +[](sqlite3_context* ctx, int len, sqlite3_value** args)
      {
        auto cc = context<Args...>(ctx);
        auto aa =  reinterpret_cast<value*>(args);
        auto &f = *reinterpret_cast<func_type*>(sqlite3_user_data(ctx));

        try
        {
          set_result(ctx, f(cc, boost::span<value, Extent>{aa, static_cast<std::size_t>(len)}));
        }
        BOOST_SQLITE_CATCH_RESULT(ctx)

      }, nullptr, nullptr,
      +[](void * ptr) noexcept {delete static_cast<func_type*>(ptr);}
      );
}

template<typename Func, typename ... Args, std::size_t Extent>
auto create_scalar_function_impl(sqlite3 * db,
                          const std::string & name,
                          Func && func,
                          std::tuple<context<Args...>, boost::span<value, Extent>> * ,
                          std::true_type /* void return */,
                          std::false_type /* is pointer */) -> int
{
  using func_type = typename std::decay<Func>::type;
  return sqlite3_create_function_v2(
      db,
      name.c_str(),
      (Extent == boost::dynamic_extent) ? -1 : static_cast<int>(Extent),
      SQLITE_UTF8,
      new func_type(std::forward<Func>(func)),
      +[](sqlite3_context* ctx, int len, sqlite3_value** args)
      {
        auto cc = context<Args...>(ctx);
        auto aa =  reinterpret_cast<value*>(args);
        auto &f = *reinterpret_cast<func_type*>(sqlite3_user_data(ctx));

        try
        {
          f(cc, boost::span<value, Extent>{aa, static_cast<std::size_t>(len)});
        }
        BOOST_SQLITE_CATCH_RESULT(ctx)

      }, nullptr, nullptr,
      +[](void * ptr){delete static_cast<func_type*>(ptr);}
      );
}


template<typename Func, typename ... Args, std::size_t Extent>
auto create_scalar_function_impl(sqlite3 * db,
                          const std::string & name,
                          Func && func,
                          std::tuple<context<Args...>, boost::span<value, Extent>> * ,
                          std::false_type /* void return */,
                          std::true_type /* is pointer */) -> int
{
  return sqlite3_create_scalar_function_v2(
      db, name.c_str(),
      Extent == boost::dynamic_extent ? -1 : Extent,
      SQLITE_UTF8,
      func,
      +[](sqlite3_context* ctx, int len, sqlite3_value** args)
      {
        auto cc = context<Args...>(ctx);
        auto aa =  reinterpret_cast<value*>(args);
        auto  f = *reinterpret_cast<Func*>(sqlite3_user_data(ctx));

        try
        {
          set_result(ctx, f(cc, boost::span<value, Extent>{aa, static_cast<std::size_t>(len)}));
        }
        BOOST_SQLITE_CATCH_RESULT(ctx)


      }, nullptr, nullptr,
      nullptr
      );
}

template<typename Func, typename ... Args, std::size_t Extent>
auto create_scalar_function_impl(sqlite3 * db,
                                 const std::string & name,
                                 Func && func,
                                 std::tuple<context<Args...>, boost::span<value, Extent>> * ,
                                 std::true_type /* void return */,
                                 std::true_type /* is pointer */) -> int
{
  return sqlite3_create_function_v2(
      db, name.c_str(),
      Extent == boost::dynamic_extent ? -1 : Extent,
      SQLITE_UTF8, func,
      +[](sqlite3_context* ctx, int len, sqlite3_value** args)
      {
        auto cc = context<Args...>(ctx);
        auto aa =  reinterpret_cast<value*>(args);
        auto  f = *reinterpret_cast<Func*>(sqlite3_user_data(ctx));

        try
        {
          f(cc, boost::span<value, Extent>{aa, static_cast<std::size_t>(len)});
        }
        BOOST_SQLITE_CATCH_RESULT(ctx)


      }, nullptr, nullptr,
      nullptr
      );
}

template<typename Func>
auto create_scalar_function(sqlite3 * db,
                     const std::string & name,
                     Func && func)
                     -> decltype(create_scalar_function_impl(
                         db, name, std::forward<Func>(func),
                         static_cast<callable_traits::args_t<Func>*>(nullptr),
                         callable_traits::has_void_return<Func>{},
                         std::is_pointer<typename std::decay<Func>::type>{}
                         ))
{
  return create_scalar_function_impl(db, name, std::forward<Func>(func),
                                     static_cast<callable_traits::args_t<Func>*>(nullptr),
                                     callable_traits::has_void_return<Func>{},
                                     std::is_pointer<typename std::decay<Func>::type>{});
}


template<typename Func>
auto create_aggregate_function(sqlite3 * db,
                               const std::string & name,
                               Func && func)
{
  using args_type    = callable_traits::args_t<decltype(&Func::step)>;
  using result_type  = callable_traits::return_type<decltype(&Func::final)>;
  using context_type = typename std::remove_reference<typename std::tuple_element<1u, args_type>::type>::type;
  using span_type    = typename std::tuple_element<2U, args_type>::type;
  using func_type    = typename std::decay<Func>::type;

  return sqlite3_create_function_v2(
      db, name.c_str(),
      span_type::extent == boost::dynamic_extent ? -1 : static_cast<int>(span_type::extent),
      SQLITE_UTF8,
      new func_type(std::forward<Func>(func)),
      nullptr,
      +[](sqlite3_context* ctx, int len, sqlite3_value** args)
      {
        auto aa =  reinterpret_cast<value*>(args);
        auto  f = reinterpret_cast<Func*>(sqlite3_user_data(ctx));
        auto c = static_cast<context_type*>(sqlite3_aggregate_context(ctx, 0));

        try
        {
          if (c == nullptr)
          {
            auto p = sqlite3_aggregate_context(ctx, sizeof(context_type));
            c = new (p) context_type();
          }
          f->step(*c, span_type{aa, static_cast<std::size_t>(len)});
        }
        BOOST_SQLITE_CATCH_RESULT(ctx)
      },
      [](sqlite3_context* ctx)
      {
        auto  f = reinterpret_cast<Func*>(sqlite3_user_data(ctx));
        auto c = static_cast<context_type*>(sqlite3_aggregate_context(ctx, 0));

        try
        {
          if (c == nullptr)
          {
            auto p = sqlite3_aggregate_context(ctx, sizeof(context_type));
            c = new (p) context_type();
          }
          set_result(ctx, f->final(*c));
          c->~context_type();
        }
        BOOST_SQLITE_CATCH_RESULT(ctx)
      },
      [](void * ptr) noexcept { delete static_cast<func_type*>(ptr);}
  );
}

template<typename Func>
auto create_window_function(sqlite3 * db,
                            const std::string & name,
                            Func && func)
{
  using args_type    = callable_traits::args_t<decltype(&Func::step)>;
  using result_type  = callable_traits::return_type<decltype(&Func::value)>;
  using context_type = typename std::remove_reference<typename std::tuple_element<1u, args_type>::type>::type;
  using span_type    = typename std::tuple_element<2U, args_type>::type;
  using func_type    = typename std::decay<Func>::type;

  return sqlite3_create_window_function(
      db, name.c_str(),
      span_type::extent == boost::dynamic_extent ? -1 : static_cast<int>(span_type::extent),
      SQLITE_UTF8,
      new func_type(std::forward<Func>(func)),
      +[](sqlite3_context* ctx, int len, sqlite3_value** args) noexcept //xStep
      {
        auto aa =  reinterpret_cast<value*>(args);
        auto  f = reinterpret_cast<Func*>(sqlite3_user_data(ctx));
        auto c = static_cast<context_type*>(sqlite3_aggregate_context(ctx, 0));
        try
        {
          if (c == nullptr)
          {
            auto p = sqlite3_aggregate_context(ctx, sizeof(context_type));
            c = new (p) context_type();
          }
          f->step(*c, span_type{aa, static_cast<std::size_t>(len)});
        }
        BOOST_SQLITE_CATCH_RESULT(ctx)
      },
      [](sqlite3_context* ctx) // xFinal
      {
        auto  f = reinterpret_cast<Func*>(sqlite3_user_data(ctx));

        auto c = static_cast<context_type*>(sqlite3_aggregate_context(ctx, 0));
        try
        {
          if (c == nullptr)
          {
            auto p = sqlite3_aggregate_context(ctx, sizeof(context_type));
            c = new (p) context_type();
          }
          set_result(ctx, f->value(*c));
          c->~context_type();
        }
        BOOST_SQLITE_CATCH_RESULT(ctx)
      },
      [](sqlite3_context* ctx) //xValue
      {
        auto  f = reinterpret_cast<Func*>(sqlite3_user_data(ctx));
        auto c = static_cast<context_type*>(sqlite3_aggregate_context(ctx, 0));
        try
        {
          if (c == nullptr)
          {
            auto p = sqlite3_aggregate_context(ctx, sizeof(context_type));
            c = new (p) context_type();
          }
          set_result(ctx, f->value(*c));
        }
        BOOST_SQLITE_CATCH_RESULT(ctx)
      },
      +[](sqlite3_context* ctx, int len, sqlite3_value** args) // xInverse
      {
        auto aa =  reinterpret_cast<value*>(args);
        auto  f = reinterpret_cast<Func*>(sqlite3_user_data(ctx));
        auto c = static_cast<context_type*>(sqlite3_aggregate_context(ctx, 0));
        try
        {
          if (c == nullptr)
          {
            auto p = sqlite3_aggregate_context(ctx, sizeof(context_type));
            c = new (p) context_type();
          }
          f->inverse(*c, span_type{aa, static_cast<std::size_t>(len)});
        }
        BOOST_SQLITE_CATCH_RESULT(ctx)

      },
      [](void * ptr) /* xDestroy */ { delete static_cast<func_type*>(ptr);}
  );
}

}

///@{
/** @brief create a scalar function
   @ingroup reference

 @param conn The connection to add the function to.
 @param name The name of the function
 @param func The function to be added
 @param ec The error_code

 @throws `system::system_error` when the overload without `ec` is used.

 `func` must take `context<Args...>` as the first and a `span<value, N>` as the second value.
 If `N` is not `dynamic_extent` it will be used to deduce the number of arguments for the function.

 @par Example

 @code{.cpp}
  extern sqlite::connection conn;

  sqlite::create_function(
    conn, "my_sum",,
    [](sqlite::context<std::size_t> ctx,
       boost::span<sqlite::value, 1u> args) -> std::size_t
    {
        auto value = args[0].get_int64();
        auto p = ctx.get_if<0>();
        if (p != nullptr) // increment the counter
            return (*p) += value;
        else // set the initial value
            ctx.set<0>(value);
        return value;
    });
  @endcode

 */
template<typename Func>
auto create_scalar_function(
    connection & conn,
    const std::string & name,
    Func && func,
    system::error_code & ec)
#if !defined(BOOST_SQLITE_GENERATING_DOCS)
    -> typename std::enable_if<
        std::is_same<
          decltype(
              detail::create_scalar_function(
                  static_cast<sqlite3*>(nullptr), name,
                  std::declval<Func>())
              ), int>::value>::type
#endif
{
    auto res = detail::create_scalar_function(conn.handle(), name, std::forward<Func>(func));
    if (res != 0)
        BOOST_SQLITE_ASSIGN_EC(ec, res);
}

///@{
/** @brief create a scalar function
   @ingroup reference

 @param conn The connection to add the function to.
 @param name The name of the function
 @param func The function to be added
 @param ec The error_code

 @throws `system::system_error` when the overload without `ec` is used.

 `func` must take `context<Args...>` as the first and a `span<value, N>` as the second value.
 If `N` is not `dynamic_extent` it will be used to deduce the number of arguments for the function.

 @par Example

 @code{.cpp}
  extern sqlite::connection conn;

  sqlite::create_function(
    conn, "to_upper",
    [](sqlite::context<> ctx,
       boost::span<sqlite::value, 1u> args) -> std::string
    {
        std::string res;
        auto txt = val[0].get_text();
        res.resize(txt.size());
        std::transform(txt.begin(), txt.end(), res.begin(),
                       [](char c){return std::toupper(c);});
        return value;
    });
  @endcode

 */
template<typename Func>
auto create_scalar_function(
    connection & conn,
    const std::string & name,
    Func && func)
    -> typename std::enable_if<
        std::is_same<
          decltype(
              detail::create_scalar_function(
                  static_cast<sqlite3*>(nullptr), name,
                  std::declval<Func>())
              ), int>::value>::type
{
    system::error_code ec;
    create_scalar_function(conn, name, std::forward<Func>(func), ec);
    if (ec)
        throw_exception(system::system_error(ec));
}
///@}


///@{
/** @brief create a aggregate function
   @ingroup reference

 @param conn The connection to add the function to.
 @param name The name of the function
 @param func The function to be added
 @param ec The error_code

 @throws `system::system_error` when the overload without `ec` is used.


 `func` needs to be an object with two functions:

 @code{.cpp}
 void step(State &, boost::span<sqlite::value, N> args);
 T final(State &);
 @endcode

 `State` can be any type and will get deduced together with `N`.
 An aggregrate function will create a new `State` for a new `aggregate` and call `step` for every step.
 When the aggregation is done `final` is called and the result is returned to sqlite.

 @par Example

 @code{.cpp}
  extern sqlite::connection conn;

  struct aggregate_func
  {
      std::size_t counter;
      void step(std::size_t & counter, boost::span<sqlite::value, 1u> val)
      {
        counter += val[0].get_text().size();
      }

      std::size_t final(std::size_t & counter)
      {
        return counter;
      }
  };

  sqlite::create_function(
    conn, "char_counter",
    aggregate_func{});

  @endcode

 */
template<typename Func>
void create_aggregate_function(
    connection & conn,
    const std::string & name,
    Func && func,
    system::error_code & ec,
    error_info & ei)
{
    auto res = detail::create_aggregate_function(conn.handle(), name, std::forward<Func>(func));
    if (res != 0)
    {
        BOOST_SQLITE_ASSIGN_EC(ec, res);
        ei.set_message(sqlite3_errmsg(conn.handle()));
    }
}

template<typename Func>
void create_aggregate_function(
    connection & conn,
    const std::string & name,
    Func && func)
{
    system::error_code ec;
    error_info ei;
    create_aggregate_function(conn, name, std::forward<Func>(func), ec, ei);
    if (ec)
        throw_exception(system::system_error(ec, ei.message()));
}
///@}


///@{
/** @brief create a aggregate window function
   @ingroup reference

 @param conn The connection to add the function to.
 @param name The name of the function
 @param func The function to be added
 @param ec The error_code

 @throws `system::system_error` when the overload without `ec` is used.

 `func` needs to be an object with three functions:

 @code{.cpp}
 void step(State &, boost::span<sqlite::value, N> args);
 void inverse(State & , boost::span<sqlite::value, N> args);
 T final(State &);
 @endcode

 `State` can be any type and will get deduced together with `N`.
 An aggregrate function will create a new `State` for a new `aggregate` and call `step` for every step.
 When an element is removed from the window `inverse` is called.
 When the aggregation is done `final` is called and the result is returned to sqlite.

 @par Example

 @code{.cpp}
  extern sqlite::connection conn;

  struct window_func
  {
      std::size_t counter;
      void step(std::size_t & counter, boost::span<sqlite::value, 1u> val)
      {
        counter += val[0].get_text().size();
      }

      void inverse(std::size_t & counter, boost::span<sqlite::value, 1u> val)
      {
        counter -= val[0].get_text().size();
      }

      std::size_t final(std::size_t & counter)
      {
        return counter;
      }
  };

  sqlite::create_function(
    conn, "win_char_counter",
    aggregate_func{});
  @endcode
 */
template<typename Func>
void create_window_function(
    connection & conn,
    const std::string & name,
    Func && func,
    system::error_code & ec)
{
    auto res = detail::create_window_function(conn.handle(), name, std::forward<Func>(func));
    if (res != 0)
        BOOST_SQLITE_ASSIGN_EC(ec, res);
}

template<typename Func>
void create_window_function(
    connection & conn,
    const std::string & name,
    Func && func)
{
    system::error_code ec;
    create_window_function(conn, name, std::forward<Func>(func), ec);
    if (ec)
        throw_exception(system::system_error(ec));
}

///@}

BOOST_SQLITE_END_NAMESPACE

#endif //BOOST_SQLITE_FUNCTION_HPP
