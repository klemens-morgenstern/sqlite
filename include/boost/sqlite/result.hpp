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


BOOST_SQLITE_BEGIN_NAMESPACE


struct set_result_tag {};


inline void tag_invoke(set_result_tag, sqlite3_context * ctx, blob b)
{
  auto sz = b.size();
  sqlite3_result_blob(ctx, std::move(b).release(), sz, &operator delete);
}

inline void tag_invoke(set_result_tag, sqlite3_context * ctx, double dbl) { sqlite3_result_double(ctx, dbl); }

template<typename I,
    typename = std::enable_if_t<std::is_integral<I>::value>>
inline void tag_invoke(set_result_tag, sqlite3_context * ctx, I value)
{
  BOOST_IF_CONSTEXPR ((sizeof(I) == sizeof(int) && std::is_unsigned<I>::value)  || (sizeof(I) > sizeof(int)))
    sqlite3_result_int64(ctx, static_cast<sqlite3_int64>(value));
  else
    sqlite3_result_int(ctx, static_cast<int>(value));
}

inline void tag_invoke(set_result_tag, sqlite3_context * ctx, std::nullptr_t) { sqlite3_result_null(ctx); }
inline void tag_invoke(set_result_tag, sqlite3_context * ctx, string_view str)
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

inline void tag_invoke(set_result_tag, sqlite3_context * ctx, variant2::monostate) { }
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
