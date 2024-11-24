//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_RESULT_HPP
#define BOOST_SQLITE_RESULT_HPP

#include <boost/sqlite/blob.hpp>
#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/value.hpp>

#include <boost/variant2/variant.hpp>

#include <type_traits>


BOOST_SQLITE_BEGIN_NAMESPACE


struct set_result_tag {};


inline void tag_invoke(set_result_tag, sqlite3_context * ctx, blob b)
{
  auto sz = b.size();
  sqlite3_result_blob(ctx, std::move(b).release(), sz, &operator delete);
}


inline void tag_invoke(set_result_tag, sqlite3_context * ctx, zero_blob zb)
{
  sqlite3_result_zeroblob64(ctx, static_cast<sqlite3_uint64>(zb));
}

inline void tag_invoke(set_result_tag, sqlite3_context * ctx, double dbl) { sqlite3_result_double(ctx, dbl); }

inline void tag_invoke(set_result_tag, sqlite3_context * ctx, sqlite3_int64 value)
{
  sqlite3_result_int64(ctx, static_cast<sqlite3_int64>(value));
}

template<typename = std::enable_if_t<!std::is_same<std::int64_t, sqlite3_int64>::value>>
inline void tag_invoke(set_result_tag, sqlite3_context * ctx, std::int64_t value)

{
  sqlite3_result_int64(ctx, static_cast<sqlite3_int64>(value));
}

inline void tag_invoke(set_result_tag, sqlite3_context * ctx, std::nullptr_t) { sqlite3_result_null(ctx); }
inline void tag_invoke(set_result_tag, sqlite3_context * ctx, string_view str)
{
  sqlite3_result_text(ctx, str.data(), str.size(), SQLITE_TRANSIENT);
}
template<typename String>
inline auto tag_invoke(set_result_tag, sqlite3_context * ctx, String && str)
  -> typename std::enable_if<std::is_convertible<String, string_view>::value>::type
{
  return tag_invoke(set_result_tag{}, ctx, string_view(str));
}


inline void tag_invoke(set_result_tag, sqlite3_context * , variant2::monostate) { }
inline void tag_invoke(set_result_tag, sqlite3_context * ctx, const value & val)
{
  sqlite3_result_value(ctx, val.handle());
}

struct set_variant_result
{
  sqlite3_context * ctx;
  template<typename T>
  void operator()(T && val)
  {
    tag_invoke(set_result_tag{}, ctx, std::forward<T>(val));
  }
};

template<typename ... Args>
inline void tag_invoke(set_result_tag, sqlite3_context * ctx, const variant2::variant<Args...> & var)
{
  visit(set_variant_result{ctx}, var);
}

template<typename T>
inline void tag_invoke(set_result_tag, sqlite3_context * ctx, std::unique_ptr<T> ptr)
{
  sqlite3_result_pointer(ctx, ptr.release(), typeid(T).name(), +[](void * ptr){delete static_cast<T*>(ptr);});
}

template<typename T>
inline void tag_invoke(set_result_tag, sqlite3_context * ctx, std::unique_ptr<T, void(*)(T*)> ptr)
{
  sqlite3_result_pointer(ctx, ptr.release(), typeid(T).name(), static_cast<void(*)(void*)>(ptr.get_deleter()));
}

template<typename T, typename Deleter>
inline auto tag_invoke(set_result_tag, sqlite3_context * ctx, std::unique_ptr<T> ptr)
    -> typename std::enable_if<std::is_empty<Deleter>::value &&
                               std::is_default_constructible<Deleter>::value>::type
{
  sqlite3_result_pointer(ctx, ptr.release(), typeid(T).name(), +[](void * ptr){Deleter()(static_cast<T*>(ptr));});
}

inline void tag_invoke(set_result_tag, sqlite3_context * ctx, error err)
{
  if (err.info)
    sqlite3_result_error(ctx, err.info.message().c_str(), -1);
  sqlite3_result_error_code(ctx, err.code);
}


template<typename T>
inline void tag_invoke(set_result_tag tag, sqlite3_context * ctx, result<T> res)
{
  if (res.has_value())
    tag_invoke(tag, ctx, std::move(res).value());
  else
    tag_invoke(tag, ctx, std::move(res).error());
}

inline void tag_invoke(set_result_tag tag, sqlite3_context * ctx, result<void> res)
{
  if (res.has_error())
    tag_invoke(tag, ctx, std::move(res).error());
}


namespace detail
{

template<typename Value>
inline auto set_result(sqlite3_context * ctx, Value && value)
    -> decltype(tag_invoke(set_result_tag{}, ctx, std::forward<Value>(value)))
{
  tag_invoke(set_result_tag{}, ctx, std::forward<Value>(value));
}


}


BOOST_SQLITE_END_NAMESPACE

#endif //BOOST_SQLITE_RESULT_HPP
