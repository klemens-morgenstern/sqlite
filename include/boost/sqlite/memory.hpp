// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_MEMORY_HPP
#define BOOST_SQLITE_MEMORY_HPP

#include <boost/sqlite/detail/config.hpp>

#include <memory>

BOOST_SQLITE_BEGIN_NAMESPACE
/// A tag to allow `operator new`
/// @ingroup reference
struct memory_tag {};
BOOST_SQLITE_END_NAMESPACE


inline void *operator new  ( std::size_t size, boost::sqlite::memory_tag) noexcept
{
  using namespace boost::sqlite;
  return sqlite3_malloc64(size);
}
inline void *operator new[]( std::size_t size, boost::sqlite::memory_tag) noexcept
{
  using namespace boost::sqlite;
  return sqlite3_malloc64(size);
}

inline void operator delete  ( void* ptr, boost::sqlite::memory_tag) noexcept
{
  using namespace boost::sqlite;
  return sqlite3_free(ptr);
}

BOOST_SQLITE_BEGIN_NAMESPACE

template<typename T>
void delete_(T * t)
{
  struct scoped_free
  {
    void * p;
    ~scoped_free()
    {
      sqlite3_free(p);
    }
  };
  scoped_free _{t};
  t->~T();
}

namespace detail
{

template<typename T>
struct deleter
{
  void operator()(T* t)
  {
    delete_(t);
  }
};

template<typename T>
struct deleter<T[]>
{
  static_assert(std::is_trivially_destructible<T>::value, "T[] needs to be trivially destructible");
  void operator()(T* t)
  {
    sqlite3_free(t);
  }
};

template<>
struct deleter<void>
{
  void operator()(void* t)
  {
    sqlite3_free(t);
  }
};
}

template<typename T>
using unique_ptr = std::unique_ptr<T, detail::deleter<T>>;

template<typename T>
inline std::size_t msize(const unique_ptr<T> & ptr)
{
  return sqlite3_msize(ptr.get());
}

template<typename T, typename ... Args>
unique_ptr<T> make_unique(Args && ... args)
{
  unique_ptr<void> up{sqlite3_malloc64(sizeof(T))};
  unique_ptr<T> res{new (up.get()) T(std::forward<Args>(args)...)};
  up.release();
  return res;
}

BOOST_SQLITE_END_NAMESPACE

#endif //BOOST_SQLITE_MEMORY_HPP
