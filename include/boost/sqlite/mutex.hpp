//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_MUTEX_HPP
#define BOOST_SQLITE_MUTEX_HPP

#include <boost/sqlite/detail/config.hpp>
#include <memory>

BOOST_SQLITE_BEGIN_NAMESPACE
/// A mutex class that maybe a noop depending on the mode sqlite3 was compiled as.
struct mutex
{
  bool try_lock()
  {
    if (!impl_)
      return false;
    return sqlite3_mutex_try(impl_.get()) == SQLITE_OK;
  }
  void lock()   { sqlite3_mutex_enter(impl_.get()); }
  void unlock() { sqlite3_mutex_leave(impl_.get()); }

  mutex() : impl_(sqlite3_mutex_alloc(SQLITE_MUTEX_FAST)) {}
  mutex(mutex && ) = delete;
 private:
  struct deleter_ {void operator()(sqlite3_mutex *mtx) {sqlite3_mutex_free(mtx);}};
  std::unique_ptr<sqlite3_mutex, deleter_> impl_;
};

/// A recursive mutex class that maybe a noop depending on the mode sqlite3 was compiled as.
struct recursive_mutex
{
  bool try_lock()
  {
    if (!impl_)
      return false;
    return sqlite3_mutex_try(impl_.get()) == SQLITE_OK;
  }
  void lock()   { sqlite3_mutex_enter(impl_.get()); }
  void unlock() { sqlite3_mutex_leave(impl_.get()); }

  recursive_mutex() : impl_(sqlite3_mutex_alloc(SQLITE_MUTEX_RECURSIVE)) {}
  recursive_mutex(recursive_mutex && ) = delete;
 private:
  struct deleter_ {void operator()(sqlite3_mutex *mtx) {sqlite3_mutex_free(mtx);}};
  std::unique_ptr<sqlite3_mutex, deleter_> impl_;
};

BOOST_SQLITE_END_NAMESPACE

#endif //BOOST_SQLITE_MUTEX_HPP
