//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_HOOKS_HPP
#define BOOST_SQLITE_HOOKS_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/function.hpp>
#include <boost/system/result.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE


#if defined(SQLITE_ENABLE_PREUPDATE_HOOK)
/** The context for pre-update events

 @note This is only available if sqlite was compiled with `SQLITE_ENABLE_PREUPDATE_HOOK` enabled.

 */
struct preupdate_context
{
  /// Get the old value, i.e. the value before the update.
  system::result<value> old(int column) const
  {
    sqlite3_value * val;
    int res = sqlite3_preupdate_old(db_, column, &val);
    if (res != 0)
      BOOST_SQLITE_RETURN_EC(res);
    return value(val);
  }
  /// The count of colums to be updated
  int count() const { return sqlite3_preupdate_count(db_); }
  /// The nesting depth of the update.
  int depth() const { return sqlite3_preupdate_depth(db_); }
  /// The new value to be written to column
  system::result<value> new_(int column) const
  {
    sqlite3_value * val;
    int res = sqlite3_preupdate_new(db_, column, &val);
    if (res != 0)
      BOOST_SQLITE_RETURN_EC(res);
    return value(val);
  }

  /// @brief Query the status of blob access, e.g. when using @ref blob_handle
  /// @see https://www.sqlite.org/c3ref/preupdate_blobwrite.html
  int blob_write() const { return sqlite3_preupdate_blobwrite(db_); }

  explicit preupdate_context(sqlite3 * db) noexcept : db_(db) {}
 private:
  sqlite3 * db_;
};

#endif

namespace detail
{


template<typename Func>
bool commit_hook_impl(sqlite3 * db,
                      Func * func,
                      std::true_type)
{
  static_assert(noexcept(func()));
  return sqlite3_commit_hook( db, func, [](void * data) { return (*static_cast<Func *>(data))() ? 1 : 0; }, nullptr) != nullptr;
}


template<typename Func>
bool commit_hook_impl(sqlite3 * db,
                      Func && func,
                      std::false_type)
{
  static_assert(noexcept(func()));
  using func_type    = typename std::decay<Func>::type;

  return sqlite3_commit_hook(
      db,
      [](void * data) { return (*static_cast<Func *>(data))() ? 1 : 0; },
      &func) != nullptr;
}

inline bool commit_hook(sqlite3 * db, std::nullptr_t, std::false_type)
{
  return sqlite3_commit_hook(db, nullptr, nullptr);
}



template<typename Func>
bool commit_hook(sqlite3 * db,
                 Func && func)
{
  using func_type    = typename std::decay<Func>::type;
  return commit_hook_impl(db, std::forward<Func>(func), std::is_pointer<func_type>{});
}



template<typename Func>
bool rollback_hook_impl(sqlite3 * db,
                      Func * func,
                      std::true_type)
{
  static_assert(noexcept(func()));
  return sqlite3_rollback_hook( db, func, [](void * data) { (*static_cast<Func *>(data))(); }, nullptr) != nullptr;
}


template<typename Func>
bool rollback_hook_impl(sqlite3 * db,
                      Func && func,
                      std::false_type)
{
  static_assert(noexcept(func()));
  using func_type    = typename std::decay<Func>::type;

  return sqlite3_rollback_hook(
      db,
      [](void * data) { (*static_cast<Func *>(data))(); },
      &func) != nullptr;
}

inline bool rollback_hook_impl(sqlite3 * db, std::nullptr_t, std::false_type)
{
  return sqlite3_rollback_hook(db, nullptr, nullptr);
}

template<typename Func>
bool rollback_hook(sqlite3 * db,
                 Func && func)
{
  using func_type    = typename std::decay<Func>::type;
  return rollback_hook_impl(db, std::forward<Func>(func), std::is_pointer<func_type>{});
}

#if defined(SQLITE_ENABLE_PREUPDATE_HOOK)

template<typename Func>
bool preupdate_hook_impl(sqlite3 * db,
                      Func * func,
                      std::true_type)
{
  static_assert(noexcept(func(preupdate_context(nullptr), SQLITE_SELECT, "", "", 0, 0)));
  return sqlite3_preupdate_hook(
      db, func,
      [](void * data,
         sqlite3* db,
         int op,
         const char * db_name,
         const char * table_name,
         sqlite_int64 key1,
         sqlite_int64 key2)
      {
        (*static_cast<Func *>(data))(preupdate_context(db), op, db_name, table_name, key1, key2);
      }, nullptr) != nullptr;
}


template<typename Func>
bool preupdate_hook_impl(sqlite3 * db,
                         Func & func,
                         std::false_type)
{
  static_assert(noexcept(func(preupdate_context(nullptr), SQLITE_SELECT, "", "", 0, 0)),
                "hooks but be noexcept");
  using func_type    = typename std::decay<Func>::type;

  return sqlite3_preupdate_hook(
      db,
      [](void * data,
         sqlite3* db,
         int op,
         const char * db_name,
         const char * table_name,
         sqlite_int64 key1,
         sqlite_int64 key2)
      {
        (*static_cast<Func *>(data))(preupdate_context(db), op, db_name, table_name, key1, key2);
      }, &func) != nullptr;
}

inline bool preupdate_hook_impl(sqlite3 * db, std::nullptr_t, std::false_type)
{
  return sqlite3_preupdate_hook(db, nullptr, nullptr);
}

template<typename Func>
bool preupdate_hook(sqlite3 * db,
                     Func && func)
{
  using func_type    = typename std::decay<Func>::type;
  return preupdate_hook_impl(db, std::forward<Func>(func), std::is_pointer<func_type>{});
}

#endif

template<typename Func>
bool update_hook_impl(sqlite3 * db,
                      Func * func,
                      std::true_type)
{
  static_assert(noexcept(func(SQLITE_SELECT, "", "", 0)));
  return sqlite3_update_hook(
      db, func,
      [](void * data,
         sqlite3*,
         int op,
         const char * db,
         const char * name,
         sqlite_int64 key)
      {
        (*static_cast<Func *>(data))(op, db, name, key);
      }, nullptr) != nullptr;
}


template<typename Func>
bool update_hook_impl(sqlite3 * db,
                      Func & func,
                      std::false_type)
{
  static_assert(noexcept(func(SQLITE_SELECT, "", "", 0)));
  using func_type    = typename std::decay<Func>::type;

  return sqlite3_update_hook(
      db,
      [](void * data,
         int op,
         const char * db,
         const char * name,
         sqlite_int64 key)
      {
        (*static_cast<func_type*>(data))(op, db, name, key);
      }, &func) != nullptr;
}

inline
bool update_hook_impl(sqlite3 * db,
                      std::nullptr_t,
                      std::false_type)
{
  return sqlite3_update_hook(db, nullptr, nullptr);
}

template<typename Func>
bool update_hook(sqlite3 * db,
                 Func && func)
{
  using func_type    = typename std::decay<Func>::type;
  return update_hook_impl(db, std::forward<Func>(func), std::is_pointer<func_type>{});
}


}

/**
  @brief Install a commit hook
  @ingroup reference

  @see [related sqlite documentation](https://www.sqlite.org/c3ref/commit_hook.html)

  The commit hook gets called before a commit gets performed.
  If `func` returns true, the commit goes, otherwise it gets rolled back.

  @note If the function is not a free function pointer, this function will *NOT* take ownership.

  @note If `func` is a `nullptr` the hook gets reset.

  @param conn The database connection to install the hook in
  @param func The hook function
  @return true if an hook has been replaced.
 */
template<typename Func>
bool commit_hook(connection & conn, Func && func)
{
  return detail::commit_hook(conn.handle(), std::forward<Func>(func));
}

/**
  @brief Install a rollback hook
  @ingroup reference

  @see [related sqlite documentation](https://www.sqlite.org/c3ref/commit_hook.html)

  The rollback hook gets called when a rollback gets performed.

  @note If the function is not a free function pointer, this function will *NOT* take ownership.

  @note If `func` is a `nullptr` the hook gets reset.

  @param conn The database connection to install the hook in
  @param func The hook function
  @return true if an hook has been replaced.
 */
template<typename Func>
bool rollback_hook(connection & conn, Func && func)
{
  return detail::rollback_hook(conn.handle(), std::forward<Func>(func));
}

#if defined(SQLITE_ENABLE_PREUPDATE_HOOK)
/** A hook for pre-update events.

 @note This is only available if sqlite was compiled with `SQLITE_ENABLE_PREUPDATE_HOOK` enabled.
 @ingroup reference

 @see [related sqlite documentation](https://sqlite.org/c3ref/preupdate_count.html)

 The function will get called

 @note If the function is not a free function pointer, this function will *NOT* take ownership.

 @note If `func` is a `nullptr` the hook gets reset.

 @param conn The database connection to install the hook in
 @param func The hook function
 @return true if an hook has been replaced.

 The signature of the handler is as following (it must noexcept):

 @code{.cpp}
 void preupdate_hook(sqlite::preupdate_context ctx,
                     int op,
                     const char * db_name,
                     const char * table_name,
                     sqlite3_int64 current_key,
                     sqlite3_int64 new_key);
 @endcode



*/
template<typename Func>
bool preupdate_hook(connection & conn, Func && func)
{
  return detail::preupdate_hook(conn.handle(), std::forward<Func>(func));
}

#endif

/**
  @brief Install an update hook
  @ingroup reference

  @see [related sqlite documentation](https://www.sqlite.org/c3ref/update_hook.html)

  The update hook gets called when an update was performed.

  @note If the function is not a free function pointer, this function will *NOT* take ownership.

  The signature of the function is `void(int op, core::string_view db, core::string_view table, sqlite3_int64 id)`.
  `op` is either `SQLITE_INSERT`, `SQLITE_DELETE` and `SQLITE_UPDATE`.

  @note If `func` is a `nullptr` the hook gets reset.

  @param conn The database connection to install the hook in
  @param func The hook function
  @return true if an hook has been replaced.
 */
template<typename Func>
bool update_hook(connection & conn, Func && func)
{
  return detail::update_hook(conn.handle(), std::forward<Func>(func));
}

BOOST_SQLITE_END_NAMESPACE

#endif //BOOST_SQLITE_HOOKS_HPP
