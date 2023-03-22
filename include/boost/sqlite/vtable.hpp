//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_VTABLE_HPP
#define BOOST_SQLITE_VTABLE_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/detail/catch.hpp>
#include <boost/sqlite/function.hpp>

#include <boost/core/span.hpp>
#include <boost/core/demangle.hpp>

#include <bitset>

BOOST_SQLITE_BEGIN_NAMESPACE
namespace detail
{
struct vtab_impl;
}


namespace vtab
{

/// Helper type to set a function through the xFindFunction callback
struct function_setter
{
    /** Set the function
     *
     * @tparam Func The function type (either a lambda by ref or a pointer by copy)
     * @param func The function to be used
     *
     * The function can either take a single argument, a `span<sqlite::value, N>`
     * for scalar functions,
     * or a `context<Args...>` as first, and the span as second for aggegrate functions.
     *
     */
    template<typename Func>
    void set(Func & func)
    {
      set_impl(func, static_cast<callable_traits::args_t<Func>*>(nullptr));
    }

    template<typename ... Args, std::size_t Extent>
    void set(void(* ptr)(context<Args...>, span<value, Extent>)) noexcept
    {
      *ppArg_ = reinterpret_cast<void*>(ptr);
      *pxFunc_ =
          +[](sqlite3_context* ctx, int len, sqlite3_value** args)
          {
            auto cc = context<Args...>(ctx);
            auto aa =  reinterpret_cast<value*>(args);
            auto &f = *reinterpret_cast<void(*)(context<Args...>, span<value, Extent>)>(sqlite3_user_data(ctx));
            detail::execute_context_function(ctx, f, cc, boost::span<value, Extent>{aa, static_cast<std::size_t>(len)});
          };
    }

    template<typename T, typename ... Args,  std::size_t Extent>
    void set(T(* ptr)(context<Args...>, span<value, Extent>))
    {
      *ppArg_ = reinterpret_cast<void*>(ptr);
      *pxFunc_ =
          +[](sqlite3_context* ctx, int len, sqlite3_value** args) noexcept
          {
            auto cc = context<Args...>(ctx);
            auto aa =  reinterpret_cast<value*>(args);
            auto &f = *reinterpret_cast<T(*)(context<Args...>, span<value, Extent>)>(sqlite3_user_data(ctx));
            detail::execute_context_function(ctx, f, cc, boost::span<value, Extent>{aa, static_cast<std::size_t>(len)});
          };
    }

    template<std::size_t Extent>
    void set(void(* ptr)(span<value, Extent>))
    {
      *ppArg_ = reinterpret_cast<void*>(ptr);
      *pxFunc_ =
          +[](sqlite3_context* ctx, int len, sqlite3_value** args) noexcept
          {
            auto aa =  reinterpret_cast<value*>(args);
            auto &f = *reinterpret_cast<void(*)(span<value, Extent>)>(sqlite3_user_data(ctx));
            detail::execute_context_function(ctx, f, boost::span<value, Extent>{aa, static_cast<std::size_t>(len)});
          };
    }

    template<typename T, std::size_t Extent>
    void set(T(* ptr)(span<value, Extent>))
    {
      *ppArg_ = reinterpret_cast<void*>(ptr);
      *pxFunc_ =
          +[](sqlite3_context* ctx, int len, sqlite3_value** args) noexcept
          {
            auto aa =  reinterpret_cast<value*>(args);
            auto &f = *reinterpret_cast<T(*)(span<value, Extent>)>(sqlite3_user_data(ctx));
            detail::execute_context_function(ctx, f, boost::span<value, Extent>{aa, static_cast<std::size_t>(len)});
          };
    }

    explicit function_setter(void (**pxFunc)(sqlite3_context*,int,sqlite3_value**),
                             void **ppArg) : pxFunc_(pxFunc), ppArg_(ppArg) {}
  private:

    template<typename Func, typename ... Args, std::size_t Extent>
    void set_impl(Func & func, std::tuple<context<Args...>, span<value, Extent>>)
    {
      *ppArg_ = &func;
      *pxFunc_ =
          +[](sqlite3_context* ctx, int len, sqlite3_value** args) noexcept
          {
            auto cc = context<Args...>(ctx);
            auto aa =  reinterpret_cast<value*>(args);
            auto &f = *reinterpret_cast<Func*>(sqlite3_user_data(ctx));
            detail::execute_context_function(ctx, f, cc, boost::span<value, Extent>{aa, static_cast<std::size_t>(len)});
          };
    }
    template<typename Func, std::size_t Extent>
    void set_impl(Func & func, std::tuple<span<value, Extent>> * )
    {
      *ppArg_ = &func;
      *pxFunc_ =
          +[](sqlite3_context* ctx, int len, sqlite3_value** args) noexcept
          {
            auto aa =  reinterpret_cast<value*>(args);
            auto &f = *reinterpret_cast<Func *>(sqlite3_user_data(ctx));
            detail::execute_context_function(ctx, f, boost::span<value, Extent>{aa, static_cast<std::size_t>(len)});
          };
    }

    void (**pxFunc_)(sqlite3_context*,int,sqlite3_value**);
    void **ppArg_;
};

#if SQLITE_VERSION_NUMBER > 3038000
/** @brief Utility function that can be used in `xFilter` for the `in` operator.
    @see https://www.sqlite.org/capi3ref.html#sqlite3_vtab_in_first
    @ingroup reference
    @Note requires sqlite version >= 3.38
*/
struct in
{
    struct iterator
    {
        iterator() = default;
        explicit iterator(sqlite3_value * value ) : value_(value)
        {
            if (value == nullptr)
                return ;
            auto res = sqlite3_vtab_in_first(value, &out_.handle());
            if (res != SQLITE_OK)
            {
              system::error_code ec;
              BOOST_SQLITE_ASSIGN_EC(ec, res);
              detail::throw_error_code(ec, ec.location());
            }
        }

      iterator & operator++()
        {
          auto res = sqlite3_vtab_in_next(value_, &out_.handle());
          if (res != SQLITE_OK)
          {
            system::error_code ec;
            BOOST_SQLITE_ASSIGN_EC(ec, res);
            detail::throw_error_code(ec, ec.location());
          }
        }

      iterator operator++(int)
        {
            auto last = *this;
            ++(*this);
            return last;
        }


        const value & operator*() const
        {
            return out_;
        }

        const value * operator->() const
        {
            return &out_;
        }

        bool operator==(const iterator& other) const
        {
            return value_ == other.value_
                && out_.handle() == other.out_.handle();
        }

        bool operator!=(const iterator& other) const
        {
            return value_ != other.value_
                || out_.handle()   != other.out_.handle();
        }


      private:
        sqlite3_value * value_{nullptr};
        value out_{nullptr};
    };

    /// Returns a forward iterator to the `in` sequence for an `in` constraint pointing to the begin.
    iterator begin() {return iterator(out_);}
    /// Returns a forward iterator to the `in` sequence for an `in` constraint pointing to the end.
    iterator end() {return iterator();}

    explicit in(sqlite3_value * out) : out_(out) {}
    explicit in(sqlite::value   out) : out_(out.handle()) {}
 private:
    sqlite3_value * out_{nullptr};
};

#endif

/// index info used by the find_index function
/// @ingroup reference
/// @see [sqlite reference](https://www.sqlite.org/vtab.html#xbestindex)
struct index_info
{
  /// Returns constraints of the index.
  BOOST_ATTRIBUTE_NODISCARD
  span<const sqlite3_index_info::sqlite3_index_constraint> constraints() const
  {
    return {info_->aConstraint, static_cast<std::size_t>(info_->nConstraint)};
  }

  /// Returns ordering of the index.
  BOOST_ATTRIBUTE_NODISCARD
  span<const sqlite3_index_info::sqlite3_index_orderby> order_by() const
  {
    return {info_->aOrderBy, static_cast<std::size_t>(info_->nOrderBy)};
  }

  BOOST_ATTRIBUTE_NODISCARD
  span<sqlite3_index_info::sqlite3_index_constraint_usage> usage()
  {
    return {info_->aConstraintUsage, static_cast<std::size_t>(info_->nConstraint)};
  }

  BOOST_ATTRIBUTE_NODISCARD
  sqlite3_index_info::sqlite3_index_constraint_usage & usage_of(
      const sqlite3_index_info::sqlite3_index_constraint & info)
  {
    auto dist = std::distance(constraints().begin(), &info);
    auto itr = usage().begin() + dist;
    BOOST_ASSERT(itr < usage().end());
    return *itr;
  }

  /// Receive the collation for the contrainst of the position.
  const char * collation(std::size_t idx) const
  {
    return sqlite3_vtab_collation(info_, static_cast<int>(idx));
  }

  /// Returns true if the constraint is
  bool   distinct() const {return sqlite3_vtab_distinct(info_);}
  int on_conflict() const {return sqlite3_vtab_on_conflict(db_);}
  value * rhs_value(std::size_t idx) const
  {
    value * v;
    if (sqlite3_vtab_rhs_value(
          info_, static_cast<int>(idx),
          reinterpret_cast<sqlite3_value**>(v)) == SQLITE_OK)
      return v;
    else
      return nullptr;
  }
  void set_already_ordered() { info_->orderByConsumed = 1; }
  void set_estimated_cost(double cost) { info_->estimatedCost = cost; }
#if SQLITE_VERSION_NUMBER > 3008200
  void set_estimated_rows(sqlite3_int64 rows) { info_->estimatedRows = rows; }
#endif
#if SQLITE_VERSION_NUMBER > 3009000
  void set_index_scan_flags(int flags) { info_->idxFlags = flags; }
#endif
#if SQLITE_VERSION_NUMBER > 3010000
  std::bitset<64u> columns_used()
  {
    return std::bitset<64u>(info_->colUsed);
  }
#endif

  void set_index(int value) { info_->idxNum = value; }
  void set_index_string(char * str,
                        bool take_ownership = true)
  {
    info_->idxStr = str;
    info_->needToFreeIdxStr = take_ownership ? 1 : 0;
  }

  sqlite3_index_info * info() const { return info_; }
  sqlite3 * db() const { return db_; }

 private:
  explicit index_info(sqlite3 * db, sqlite3_index_info * info) : db_(db), info_(info) {}
  sqlite3 * db_;
  sqlite3_index_info * info_{nullptr};
  friend struct detail::vtab_impl;
};


struct module_config
{
  /// @brief Can be used to set SQLITE_VTAB_INNOCUOUS
  void set_innocuous()
  {
    sqlite3_vtab_config(db_, SQLITE_VTAB_INNOCUOUS);
  }

  /// @brief Can be used to set SQLITE_VTAB_CONSTRAINT_SUPPORT
  void set_constraint_support(bool enabled = false)
  {
    sqlite3_vtab_config(db_, SQLITE_VTAB_CONSTRAINT_SUPPORT, enabled ? 1 : 0);
  }

  /// @brief Can be used to set SQLITE_VTAB_DIRECTONLY
  void set_directonly() {sqlite3_vtab_config(db_, SQLITE_VTAB_DIRECTONLY);}

 private:
  explicit module_config(sqlite3 *db) : db_(db) {}
  friend struct detail::vtab_impl;
  sqlite3 *db_;
};

template<typename Table>
struct module
{
  using table_type = Table;

  /// @brief Creates the instance
  /// The instance_type gets used & managed by value, OR a pointer to a class that inherits sqlite3_vtab.
  /// instance_type must have a member `declaration` that returns a `const char *` for the declaration.
  BOOST_SQLITE_VIRTUAL result<table_type> create(sqlite::connection db, int argc, const char * const argv[]) BOOST_SQLITE_PURE;

  /// @brief Create a table
  /// The table_type gets used & managed by value, OR a pointer to a class that inherits sqlite3_vtab.
  /// table_type must have a member `declaration` that returns a `const char *` for the declaration.
  BOOST_SQLITE_VIRTUAL result<table_type> connect(sqlite::connection db, int argc, const char * const argv[]) BOOST_SQLITE_PURE;
};

template<typename Table>
struct eponymous_module
{
  using table_type = Table;

  /// @brief Creates the instance
  /// The instance_type gets used & managed by value, OR a pointer to a class that inherits sqlite3_vtab.
  /// instance_type must have a member `declaration` that returns a `const char *` for the declaration.
  BOOST_SQLITE_VIRTUAL result<table_type> connect(sqlite::connection db, int argc, const char * const argv[]) BOOST_SQLITE_PURE;

  eponymous_module(bool eponymous_only = false) : eponymous_only_(eponymous_only) {}

  bool eponymous_only() const {return eponymous_only_;}
 protected:
  bool eponymous_only_{false};

};

template<typename ColumnType>
struct cursor;

/// The basis for vtable
template<typename Cursor>
struct table : protected sqlite3_vtab
{
  using cursor_type = Cursor;

  BOOST_SQLITE_VIRTUAL result<void> config(module_config &) {return {};}

  /// The Table declaration to be used with  sqlite3_declare_vtab
  BOOST_SQLITE_VIRTUAL const char *declaration() BOOST_SQLITE_PURE;

  /// Destroy the storage = this function needs to be present for non eponymous tables
  BOOST_SQLITE_VIRTUAL result<void> destroy() { return {}; }

  /// Tell sqlite how to communicate with the table.
  /// Optional, this library will fill in a default function that leaves comparisons to sqlite.
  BOOST_SQLITE_VIRTUAL result<void> best_index(index_info & info) {return {system::in_place_error, SQLITE_OK};}

  /// @brief Start a search on the table.
  /// The cursor_type gets used & managed by value, OR a pointer to a class that inherits sqlite3_vtab_cursor.
  BOOST_SQLITE_VIRTUAL result<cursor_type> open() BOOST_SQLITE_PURE;

  /// Get the connection of the vtable
  sqlite::connection connection() const {return sqlite::connection{db_, false};}

  table(const sqlite::connection & conn) : db_(conn.handle()) {}
  table(sqlite3  * db = nullptr) : db_(db) {}
 private:
  template<typename ColumnType>
  friend struct cursor;
  friend struct detail::vtab_impl;

  sqlite3 * db_{nullptr};
};

/// Cursor needs the following member. @ingroup reference
template<typename ColumnType = void>
struct cursor : protected sqlite3_vtab_cursor
{
  using column_type = ColumnType;

  /// @brief Apply a filter to the cursor. Required when best_index is implemented.
  BOOST_SQLITE_VIRTUAL result<void> filter(
        int index, const char * index_data,
        boost::span<sqlite::value> values)
  {
    return {system::in_place_error, SQLITE_OK};
  }

  /// @brief Returns the next row.
  BOOST_SQLITE_VIRTUAL result<void> next() BOOST_SQLITE_PURE;

  /// @brief Check if the cursor is and the end
  BOOST_SQLITE_VIRTUAL bool eof() BOOST_SQLITE_PURE;

  /// @brief Returns the result of a value. It will use the set_result functionality to create a an sqlite function.
  /// see [vtab_no_change](https://www.sqlite.org/c3ref/vtab_nochange.html)
  BOOST_SQLITE_VIRTUAL result<column_type> column(int idx, bool no_change) BOOST_SQLITE_PURE;
  /// @brief Returns the id of the current row
  BOOST_SQLITE_VIRTUAL result<sqlite3_int64> row_id() BOOST_SQLITE_PURE;

  /// Get the table the cursor is pointing to.
        vtab::table<cursor> & table()       { return *static_cast<      vtab::table<cursor>*>(this->pVtab);}
  const vtab::table<cursor> & table() const { return *static_cast<const vtab::table<cursor>*>(this->pVtab);}

  friend struct detail::vtab_impl;
};

/// Cursor needs the following member. @ingroup reference
template<>
struct cursor<void> : protected sqlite3_vtab_cursor
{
  using column_type = void;

  /// @brief Apply a filter to the cursor. Required when best_index is implemented.
  BOOST_SQLITE_VIRTUAL result<void> filter(
      int index, const char * index_data,
      boost::span<sqlite::value> values)
  {
    return {system::in_place_error, SQLITE_OK};
  }

  /// @brief Returns the next row.
  BOOST_SQLITE_VIRTUAL result<void> next() BOOST_SQLITE_PURE;

  /// @brief Check if the cursor is and the end
  BOOST_SQLITE_VIRTUAL bool eof() BOOST_SQLITE_PURE;

  /// @brief Returns the result of a value. It will use the set_result functionality to create a an sqlite function.
  /// see [vtab_no_change](https://www.sqlite.org/c3ref/vtab_nochange.html)
  BOOST_SQLITE_VIRTUAL void column(context<> ctx, int idx, bool no_change) BOOST_SQLITE_PURE;
  /// @brief Returns the id of the current row
  BOOST_SQLITE_VIRTUAL result<sqlite3_int64> row_id() BOOST_SQLITE_PURE;

  /// Get the table the cursor is pointing to.
  vtab::table<cursor>       & table()       { return *static_cast<      vtab::table<cursor>*>(this->pVtab);}
  const vtab::table<cursor> & table() const { return *static_cast<const vtab::table<cursor>*>(this->pVtab);}

  friend struct detail::vtab_impl;
};


/// Group of functions for modifications. @ingroup reference
struct modifiable
{
  BOOST_SQLITE_VIRTUAL result<void> delete_(sqlite::value key) BOOST_SQLITE_PURE;
  /// Insert a new row
  BOOST_SQLITE_VIRTUAL result<sqlite_int64> insert(sqlite::value key, span<sqlite::value> values, int on_conflict) BOOST_SQLITE_PURE;
  /// Update the row
  BOOST_SQLITE_VIRTUAL result<sqlite_int64> update(sqlite::value old_key, sqlite::value new_key, span<sqlite::value> values, int on_conflict) BOOST_SQLITE_PURE;
};

/// Group of functions to support transactions. @ingroup reference
struct transaction
{
  /// Begin a tranasction
  BOOST_SQLITE_VIRTUAL result<void> begin()    BOOST_SQLITE_PURE;
  /// synchronize the state
  BOOST_SQLITE_VIRTUAL result<void> sync()     BOOST_SQLITE_PURE;
  /// commit the transaction
  BOOST_SQLITE_VIRTUAL result<void> commit()   BOOST_SQLITE_PURE;
  /// rollback the transaction
  BOOST_SQLITE_VIRTUAL result<void> rollback() BOOST_SQLITE_PURE;
};

/// Fucntion to enable function overriding See `xFindFunction`.
struct overload_functions
{
  /// @see https://www.sqlite.org/vtab.html#xfindfunction
  BOOST_SQLITE_VIRTUAL result<void> find_function(
          function_setter fs,
          int arg, const char * name) BOOST_SQLITE_PURE;
};

/// Make the vtable renamable @ingroup reference
struct renamable
{
  /// Function to rename the table. Optional
  BOOST_SQLITE_VIRTUAL result<void> rename(const char * new_name) BOOST_SQLITE_PURE;

};

#if SQLITE_VERSION_NUMBER >= 3007007
/// Support for recursive transactions @ingroup reference
struct recursive_transaction
{
  /// Save the current state with to `i`
  BOOST_SQLITE_VIRTUAL result<void> savepoint(int i)   BOOST_SQLITE_PURE;
  /// Release all saves states down to `i`
  BOOST_SQLITE_VIRTUAL result<void> release(int i)     BOOST_SQLITE_PURE;
  /// Roll the transaction back to `i`.
  BOOST_SQLITE_VIRTUAL result<void> rollback_to(int i) BOOST_SQLITE_PURE;
};
#endif

}


namespace detail
{

template<typename Module>
const sqlite3_module make_module(const Module & mod);

}

////@{
/** @brief Register a vtable
 @ingroup reference
 @see Sqlite vtab reference [vtab](https://www.sqlite.org/vtab.html)

 @param conn   The connection to install the vtable into
 @param name   The name for the vtable
 @param module The module to install as a vtable. See @ref vtab_module_prototype for the structure required
 @param ec     The system::error_code used to deliver errors for the exception less overload.
 @param info   The error_info used to deliver errors for the exception less overload.

 @param The requirements for `module`.

 @returns A reference to the module as stored in the database.


*/
template<typename T>
auto create_module(connection & conn,
                   cstring_ref name,
                   T && module,
                   system::error_code & ec,
                   error_info & ei) -> typename std::decay<T>::type &
{
    using module_type = typename std::decay<T>::type;
    static const sqlite3_module mod = detail::make_module(module);

    std::unique_ptr<module_type> p{new (memory_tag{}) module_type(std::forward<T>(module))};
    auto pp = p.get();

    int res = sqlite3_create_module_v2(
                             conn.handle(), name.c_str(),
                             &mod, p.release(),
                             +[](void * ptr)
                             {
                               static_cast<module_type*>(ptr)->~module_type();
                               sqlite3_free(ptr);
                             });

    if (res != SQLITE_OK)
    {
       BOOST_SQLITE_ASSIGN_EC(ec, res);
       ei.set_message(sqlite3_errmsg(conn.handle()));
    }
    return *pp;
}

template<typename T>
auto create_module(connection & conn,
                   const char * name,
                   T && module)  -> typename std::decay<T>::type &
{
    system::error_code ec;
    error_info ei;
    T & ref = create_module(conn, name, std::forward<T>(module), ec, ei);
    if (ec)
        detail::throw_error_code(ec, ei);
    return ref;
}
///@}


BOOST_SQLITE_END_NAMESPACE

#include <boost/sqlite/detail/vtable.hpp>

#endif //BOOST_SQLITE_VTABLE_HPP

