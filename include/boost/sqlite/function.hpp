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
#include <boost/sqlite/value.hpp>
#include <boost/variant2/variant.hpp>
#include <boost/callable_traits/args.hpp>
#include <boost/callable_traits/has_void_return.hpp>
#include <boost/callable_traits/return_type.hpp>

namespace boost
{
namespace sqlite
{
namespace detail
{

inline void set_result(sqlite3_context * ctx, blob b)
{
  auto sz = b.size();
  sqlite3_result_blob(ctx, std::move(b).release(), sz, &operator delete);
}

inline void set_result(sqlite3_context * ctx, double dbl) { sqlite3_result_double(ctx, dbl); }

template<typename I,
    typename = std::enable_if_t<std::is_integral<I>::value>>
inline void set_result(sqlite3_context * ctx, I value)
{
  BOOST_IF_CONSTEXPR ((sizeof(I) == sizeof(int) && std::is_unsigned<I>::value)  || (sizeof(I) > sizeof(int)))
    sqlite3_result_int64(ctx, static_cast<sqlite3_int64>(value));
  else
    sqlite3_result_int(ctx, static_cast<int>(value));
}

inline void set_result(sqlite3_context * ctx, std::nullptr_t) { sqlite3_result_null(ctx); }
inline void set_result(sqlite3_context * ctx, string_view str)
{
  auto ptr = new char[str.size()];
  std::memcpy(ptr, str.data(), str.size());
  sqlite3_result_text(
      ctx, ptr, str.size(),
      +[](void * ptr) noexcept
      {
        auto p = static_cast<char*>(ptr);
        delete[] p;
      });
}

inline void set_result(sqlite3_context * ctx, variant2::monostate) { sqlite3_result_null(ctx); }
inline void set_result(sqlite3_context * ctx, const value & val)
{
  sqlite3_result_value(ctx, val.native_handle());
}

struct set_variant_result
{
  sqlite3_context * ctx;
  template<typename T>
  void operator()(T && val)
  {
    set_result(ctx, std::forward<T>(val));
  }
};

template<typename ... Args>
inline void set_result(sqlite3_context * ctx, const variant2::variant<Args...> & var)
{
  visit(set_variant_result{ctx}, var);
}


}

template<typename ... Args>
struct context
{
  template<std::size_t Idx, typename T>
  void set(T && value)
  {
    using type = typename std::tuple_element<Idx, std::tuple<Args...>>::type;
    sqlite3_set_auxdata(ctx_, Idx, *static_cast<void**>(&value),
                        new type(std::forward<T>(value)),
                        +[](void * ptr)
                        {
                          delete static_cast<type*>(ptr);
                        });
  }

  template<std::size_t Idx>
  auto get() -> typename std::tuple_element<Idx, std::tuple<Args...>>::type &
  {
    using type = typename std::tuple_element<Idx, std::tuple<Args...>>::type;
    auto p = static_cast<type*>(sqlite3_get_auxdata(ctx_, Idx));
    if (p == nullptr)
      throw_exception(std::invalid_argument("argument not set"));
    return p;
  }

  template<std::size_t Idx>
  auto get_if() -> typename std::tuple_element<Idx, std::tuple<Args...>>::type *
  {
    using type = typename std::tuple_element<Idx, std::tuple<Args...>>::type;
    return static_cast<type*>(sqlite3_get_auxdata(ctx_, Idx));
  }

  explicit context(sqlite3_context * ctx) noexcept : ctx_(ctx) {}
  template<typename T>
  auto set_result(T && val) ->
      decltype(detail::set_result(static_cast<sqlite3_context*>(nullptr), std::forward<T>(val)))
  {
    detail::set_result(ctx_, std::forward<T>(val));
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
        catch(system::system_error & se)
        {
          auto code = SQLITE_ERROR;
          if (se.code().category() == sqlite_category())
            code = se.code().value();
          sqlite3_result_error(ctx, se.what(), code);
        }
        catch(std::exception & ex)
        {
          sqlite3_result_error(ctx, ex.what(), SQLITE_ERROR);
        }

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
        catch(system::system_error & se)
        {
          auto code = SQLITE_ERROR;
          if (se.code().category() == sqlite_category())
            code = se.code().value();
          sqlite3_result_error(ctx, se.what(), code);
        }
        catch(std::exception & ex)
        {
          sqlite3_result_error(ctx, ex.what(), SQLITE_ERROR);
        }

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
        catch(system::system_error & se)
        {
          auto code = SQLITE_ERROR;
          if (se.code().category() == sqlite_category())
            code = se.code().value();
          sqlite3_result_error(ctx, se.what(), code);
        }
        catch(std::exception & ex)
        {
          sqlite3_result_error(ctx, ex.what(), SQLITE_ERROR);
        }

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
        catch(system::system_error & se)
        {
          auto code = SQLITE_ERROR;
          if (se.code().category() == sqlite_category())
            code = se.code().value();
          sqlite3_result_error(ctx, se.what(), code);
        }
        catch(std::exception & ex)
        {
          sqlite3_result_error(ctx, ex.what(), SQLITE_ERROR);
        }

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
        catch(system::system_error & se)
        {
          auto code = SQLITE_ERROR;
          if (se.code().category() == sqlite_category())
            code = se.code().value();
          sqlite3_result_error(ctx, se.what(), code);
        }
        catch(std::exception & ex)
        {
          sqlite3_result_error(ctx, ex.what(), SQLITE_ERROR);
        }

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
        catch(system::system_error & se)
        {
          auto code = SQLITE_ERROR;
          if (se.code().category() == sqlite_category())
            code = se.code().value();
          sqlite3_result_error(ctx, se.what(), code);
        }
        catch(std::exception & ex)
        {
          sqlite3_result_error(ctx, ex.what(), SQLITE_ERROR);
        }


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
        catch(system::system_error & se)
        {
          auto code = SQLITE_ERROR;
          if (se.code().category() == sqlite_category())
            code = se.code().value();
          sqlite3_result_error(ctx, se.what(), code);
        }
        catch(std::exception & ex)
        {
          sqlite3_result_error(ctx, ex.what(), SQLITE_ERROR);
        }
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
        catch(system::system_error & se)
        {
          auto code = SQLITE_ERROR;
          if (se.code().category() == sqlite_category())
            code = se.code().value();
          sqlite3_result_error(ctx, se.what(), code);
        }
        catch(std::exception & ex)
        {
          sqlite3_result_error(ctx, ex.what(), SQLITE_ERROR);
        }
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
        catch(system::system_error & se)
        {
          auto code = SQLITE_ERROR;
          if (se.code().category() == sqlite_category())
            code = se.code().value();
          sqlite3_result_error(ctx, se.what(), code);
        }
        catch(std::exception & ex)
        {
          sqlite3_result_error(ctx, ex.what(), SQLITE_ERROR);
        }
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
        catch(system::system_error & se)
        {
          auto code = SQLITE_ERROR;
          if (se.code().category() == sqlite_category())
            code = se.code().value();
          sqlite3_result_error(ctx, se.what(), code);
        }
        catch(std::exception & ex)
        {
          sqlite3_result_error(ctx, ex.what(), SQLITE_ERROR);
        }
      },
      [](void * ptr) /* xDestroy */ { delete static_cast<func_type*>(ptr);}
  );
}

}


template<typename Func>
auto create_scalar_function(
    connection & conn,
    const std::string & name,
    Func && func,
    system::error_code & ec)
    -> typename std::enable_if<
        std::is_same<
          decltype(
              detail::create_scalar_function(
                  static_cast<sqlite3*>(nullptr), name,
                  std::declval<Func>())
              ), int>::value>::type
{
    auto res = detail::create_scalar_function(conn.native_handle(), name, std::forward<Func>(func));
    if (res != 0)
        BOOST_SQLITE_ASSIGN_EC(ec, res);
}


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

template<typename Func>
void create_aggregate_function(
    connection & conn,
    const std::string & name,
    Func && func,
    system::error_code & ec)
{
    auto res = detail::create_aggregate_function(conn.native_handle(), name, std::forward<Func>(func));
    if (res != 0)
        BOOST_SQLITE_ASSIGN_EC(ec, res);
}

template<typename Func>
void create_aggregate_function(
    connection & conn,
    const std::string & name,
    Func && func)
{
    system::error_code ec;
    create_aggregate_function(conn, name, std::forward<Func>(func), ec);
    if (ec)
        throw_exception(system::system_error(ec));
}


template<typename Func>
void create_window_function(
    connection & conn,
    const std::string & name,
    Func && func,
    system::error_code & ec)
{
    auto res = detail::create_window_function(conn.native_handle(), name, std::forward<Func>(func));
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

}
}

#endif //BOOST_SQLITE_FUNCTION_HPP
