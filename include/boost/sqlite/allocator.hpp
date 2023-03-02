//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_ALLOCATOR_HPP
#define BOOST_SQLITE_ALLOCATOR_HPP

#include <boost/sqlite/detail/config.hpp>

#include <cstddef>
#include <cstdint>

BOOST_SQLITE_BEGIN_NAMESPACE

template<typename T>
struct allocator
{
  constexpr allocator() noexcept {}
  constexpr allocator( const allocator& other ) noexcept {}
  template< class U >
  constexpr allocator( const allocator<U>& other ) noexcept {}

#if defined(SQLITE_4_BYTE_ALIGNED_MALLOC)
  constexpr static std::size_t alignment = 4u;
#else
  constexpr static std::size_t alignment = 8u;
#endif

  static_assert(alignof(T) <= alignment, "T alignment can't be fulfilled by sqlite");
  [[nodiscard]] T* allocate( std::size_t n )
  {
    return static_cast<T*>(sqlite3_malloc64(n * sizeof(T)));
  }
  void deallocate( T* p, std::size_t)
  {
    return sqlite3_free(p);
  }
};

BOOST_SQLITE_END_NAMESPACE

#endif //BOOST_SQLITE_ALLOCATOR_HPP
