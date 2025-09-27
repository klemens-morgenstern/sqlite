//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_FUNCTION_HPP
#define BOOST_SQLITE_FUNCTION_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/detail/aggregate_function.hpp>
#include <boost/sqlite/detail/scalar_function.hpp>
#include <boost/sqlite/detail/window_function.hpp>
#include <boost/sqlite/connection.hpp>
#include <boost/sqlite/result.hpp>
#include <boost/sqlite/value.hpp>
#include <boost/sqlite/detail/exception.hpp>
#include <boost/sqlite/cstring_ref.hpp>

#include <boost/core/span.hpp>
#include <boost/callable_traits/args.hpp>
#include <boost/callable_traits/has_void_return.hpp>
#include <boost/callable_traits/return_type.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE

enum function_flags
{
  deterministic  = SQLITE_DETERMINISTIC,
  directonly     = SQLITE_DIRECTONLY,
  subtype        = SQLITE_SUBTYPE,
  innocuous      = SQLITE_INNOCUOUS,
#if defined(SQLITE_RESULT_SUBTYPE)
  result_subtype = SQLITE_RESULT_SUBTYPE,
#endif
#if defined(SQLITE_SELFORDER1)
  selforder1     = SQLITE_SELFORDER1,
#else
  selforder1     = 0
#endif
};


/** @brief A context that can be passed into scalar functions.
  @ingroup reference

  @tparam Args The argument that can be stored in the context.

  @code{.cpp}
  extern sqlite::connection conn;

  sqlite::create_scalar_function(
    conn, "my_sum",
    [](sqlite::context<std::size_t> ctx,
       boost::span<sqlite::value, 1u> args) -> std::size_t
    {
        auto value = args[0].get_int();
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
                        new (memory_tag{}) element<Idx>(std::move(value)),
                        +[](void * ptr)
                        {
                          delete_(static_cast<element<Idx> *>(ptr));
                        });
  }

  /// Returns the value in the context at position `Idx`. Throws if the value isn't set.
  template<std::size_t Idx>
  auto get() -> element<Idx>  &
  {
    using type = element<Idx> ;
    auto p = static_cast<type*>(sqlite3_get_auxdata(ctx_, Idx));
    if (p == nullptr)
      detail::throw_invalid_argument("argument not set",
                                     BOOST_CURRENT_LOCATION);
    return *p;
  }

  /// Returns the value in the context at position `Idx`. Returns nullptr .value isn't set.
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
  /// Set the an error through the context, instead of throwing it.
  void set_error(cstring_ref message, int code = SQLITE_ERROR)
  {
    sqlite3_result_error(ctx_, message.c_str(), -1);
    sqlite3_result_error_code(ctx_, code);
  }
  /// Returns the connection of the context.
  connection get_connection() const
  {
    return connection{sqlite3_context_db_handle(ctx_), false};
  }

 private:
  sqlite3_context * ctx_;
};


///@{
/** @brief create a scalar function
   @ingroup reference

 @param conn The connection to add the function to.
 @param name The name of the function
 @param func The function to be added
 @param ec The system::error_code

 @throws `system::system_error` when the overload without `ec` is used.

 `func` must take `context<Args...>` as the first and a `span<value, N>` as the second value.
 If `N` is not `dynamic_extent` it will be used to deduce the number of arguments for the function.

 @par Example

 @code{.cpp}
  extern sqlite::connection conn;

  sqlite::create_function(
    conn, "my_sum",
    [](sqlite::context<std::size_t> ctx,
       boost::span<sqlite::value, 1u> args) -> std::size_t
    {
        auto value = args[0].get_int();
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
    connection_ref conn,
    cstring_ref name,
    Func && func,
    function_flags flags,
    system::error_code & ec,
    error_info & ei)
#if !defined(BOOST_SQLITE_GENERATING_DOCS)
    -> typename std::enable_if<
        std::is_same<
          decltype(
              detail::create_scalar_function(
                  static_cast<sqlite3*>(nullptr), name,
                  std::declval<Func>(), flags)
              ), int>::value>::type
#endif
{
    auto res = detail::create_scalar_function(conn.handle(), name,
                                              std::forward<Func>(func), static_cast<int>(flags));
    if (res != 0)
    {
      BOOST_SQLITE_ASSIGN_EC(ec, res);
      ei.set_message(sqlite3_errmsg(conn.handle()));
    }
}

///@{
/** @brief create a scalar function
   @ingroup reference

 @param conn The connection to add the function to.
 @param name The name of the function
 @param func The function to be added
 @param ec The system::error_code

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
    connection_ref conn,
    cstring_ref name,
    Func && func,
    function_flags flags = {})
    -> typename std::enable_if<
        std::is_same<
          decltype(
              detail::create_scalar_function(
                  static_cast<sqlite3*>(nullptr), name,
                  std::declval<Func>(), flags)
              ), int>::value>::type
{
    system::error_code ec;
    error_info ei;
    create_scalar_function(conn, name, std::forward<Func>(func), flags, ec, ei);
    if (ec)
        detail::throw_error_code(ec, ei);
}
///@}


///@{
/** @brief create a aggregate function
   @ingroup reference

 @param conn The connection to add the function to.
 @param name The name of the function
 @param args
 @param ec The system::error_code

@tparam Func The function to be added

 @throws `system::system_error` when the overload without `ec` is used.


 `func` needs to be an object with two functions:

 @code{.cpp}
 void step(boost::span<sqlite::value, N> args);
 T final();
 @endcode



 An aggregrate function will create a new `Func` for a new `aggregate` from the args tuple and call `step` for every step.
 When the aggregation is done `final` is called and the result is returned to sqlite.

 @par Example

 @code{.cpp}
  extern sqlite::connection conn;

  struct aggregate_func
  {
      aggregate_func(std::size_t init) : counter(init) {}
      std::size_t counter;
      void step(, boost::span<sqlite::value, 1u> val)
      {
        counter += val[0].get_text().size();
      }

      std::size_t final()
      {
        return counter;
      }
  };

  sqlite::create_function<aggregate_func>(
    conn, "char_counter", std::make_tuple(42));

  @endcode

 */
template<typename Func, typename Args = std::tuple<>>
void create_aggregate_function(
    connection_ref conn,
    cstring_ref name,
    Args && args,
    function_flags flags,
    system::error_code & ec,
    error_info & ei)
{
    using func_type = typename std::decay<Func>::type;
    auto res = detail::create_aggregate_function<Func>(
        conn.handle(), name, std::forward<Args>(args), static_cast<int>(flags),
        callable_traits::has_void_return<decltype(&func_type::step)>{}
        );
    if (res != 0)
    {
        BOOST_SQLITE_ASSIGN_EC(ec, res);
        ei.set_message(sqlite3_errmsg(conn.handle()));
    }
}

template<typename Func, typename Args = std::tuple<>>
void create_aggregate_function(
    connection_ref conn,
    cstring_ref name,
    Args && args= {},
    function_flags flags = {})
{
    system::error_code ec;
    error_info ei;
    create_aggregate_function<Func>(conn, name, std::forward<Args>(args), flags, ec, ei);
    if (ec)
        detail::throw_error_code(ec, ei);
}
///@}

#if SQLITE_VERSION_NUMBER >= 3025000

///@{
/** @brief create a aggregate window function
   @ingroup reference

 @param conn The connection to add the function to.
 @param name The name of the function
 @param args The arguments to construct Func from.
 @param ec The system::error_code

@tparam Func The function to be added

 @throws `system::system_error` when the overload without `ec` is used.

 `func` needs to be an object with three functions:

 @code{.cpp}
 void step(boost::span<sqlite::value, N> args);
 void inverse(boost::span<sqlite::value, N> args);
 T final();
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
      void step(boost::span<sqlite::value, 1u> val)
      {
        counter += val[0].get_text().size();
      }
      void inverse(boost::span<sqlite::value, 1u> val)
      {
        counter -= val[0].get_text().size();
      }

      std::size_t final()
      {
        return counter;
      }
  };

  sqlite::create_function(
    conn, "win_char_counter",
    aggregate_func{});
  @endcode
 */
template<typename Func, typename Args = std::tuple<>>
void create_window_function(
    connection_ref conn,
    cstring_ref name,
    Args && args,
    function_flags flags,
    system::error_code & ec)
{
    using func_type = typename std::decay<Func>::type;
    auto res = detail::create_window_function<Func>(
            conn.handle(), name, std::forward<Args>(args), static_cast<int>(flags),
            callable_traits::has_void_return<decltype(&func_type::step)>{});
    if (res != 0)
        BOOST_SQLITE_ASSIGN_EC(ec, res);
}

template<typename Func, typename Args = std::tuple<>>
void create_window_function(
    connection_ref conn,
    cstring_ref name,
    Args && args = {},
    function_flags flags = {})
{
    system::error_code ec;
    create_window_function<Func>(conn, name, std::forward<Args>(args), flags, ec);
    if (ec)
        detail::throw_error_code(ec);
}

///@}

///@{
/// Delete function

inline void delete_function(connection_ref conn, cstring_ref name, int argc, system::error_code &ec)
{
  auto res = sqlite3_create_function_v2(conn.handle(), name.c_str(), argc, 0, nullptr, nullptr, nullptr, nullptr, nullptr);
  if (res != 0)
    BOOST_SQLITE_ASSIGN_EC(ec, res);

}

inline void delete_function(connection_ref conn, cstring_ref name, int argc = -1)
{
  system::error_code ec;
  delete_function(conn, name, argc, ec);
  if (ec)
    detail::throw_error_code(ec);
}

///@}
#endif

BOOST_SQLITE_END_NAMESPACE

#endif //BOOST_SQLITE_FUNCTION_HPP
