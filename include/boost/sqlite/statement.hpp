// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_STATEMENT_HPP
#define BOOST_SQLITE_STATEMENT_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/detail/exception.hpp>
#include <boost/sqlite/blob.hpp>
#include <boost/sqlite/resultset.hpp>

#include <boost/mp11/algorithm.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/variant2/variant.hpp>


#include <tuple>

BOOST_SQLITE_BEGIN_NAMESPACE
struct connection;
template<typename, bool>
struct static_resultset;

/// @brief A reference to a value to temporary bind for an execute statement. Most values are captures by reference.
/// @ingroup reference
struct param_ref
{
    /// Default construct a parameter, gives `null`.
    param_ref() = default;
    /// Bind null
    param_ref(variant2::monostate) : impl_{variant2::in_place_type_t<variant2::monostate>{}} {}
    /// Bind null
    param_ref(std::nullptr_t)      : impl_{variant2::in_place_type_t<variant2::monostate>{}} {}
    /// Bind an integer.
    template<typename I,
             typename = typename std::enable_if<std::is_integral<I>::value>::type>
    param_ref(I value)
    {
        BOOST_IF_CONSTEXPR ((sizeof(I) == sizeof(int) && std::is_unsigned<I>::value)
                          || (sizeof(I) > sizeof(int)))
        impl_.emplace<sqlite3_int64>(static_cast<sqlite3_int64>(value));
      else
        impl_.emplace<int>(static_cast<int>(value));
    }
    /// Bind a blob.
    param_ref(blob_view blob) : impl_(blob) { }
    /// Bind a string.
    param_ref(string_view text) : impl_(text) { }

    template<typename StringLike>
    param_ref(StringLike && text,
              typename std::enable_if<std::is_constructible<string_view, StringLike>::value>::type * = nullptr)
            : impl_(variant2::in_place_type_t<string_view>{}, text) {}

    template<typename BlobLike>
    param_ref(BlobLike && text,
              typename std::enable_if<
                  !std::is_constructible<string_view, BlobLike>::value
                && std::is_constructible<blob_view, BlobLike>::value>::type * = nullptr)
        : impl_(variant2::in_place_type_t<blob_view>{}, text) {}

    /// Bind a floating point value.
    param_ref(double value) : impl_(value) { }
    /// Bind a zero_blob value, i.e. a blob that initialized by zero.
    param_ref(zero_blob zb) : impl_(zb) { }

#if SQLITE_VERSION_NUMBER >= 3020000
    /// Bind pointer value to the parameter. @see https://www.sqlite.org/bindptr.html
    template<typename T>
    param_ref(std::unique_ptr<T> ptr)
                    : impl_(variant2::in_place_index_t<7>{},
                            std::unique_ptr<void, void(*)(void*)>(
                                static_cast<void*>(ptr.release()),
                                +[](void * ptr){delete static_cast<T*>(ptr);}),
                                typeid(T).name())
    {
    }

    /// Bind pointer value with a function as deleter to the parameter. @see https://www.sqlite.org/bindptr.html
    template<typename T>
    param_ref(std::unique_ptr<T, void(*)(T*)> ptr)
                    : impl_(variant2::in_place_index_t<7>{},
                            std::unique_ptr<void, void(*)(void*)>(
                                static_cast<void*>(ptr.release()),
                                +[](void * ptr){delete static_cast<T*>(ptr);}),
                             typeid(T).name())
    {
    }

    /// @brief Bind pointer value with a function  custom deleter to the parameter.
    /// The deleter needs to be default constructible. @see https://www.sqlite.org/bindptr.html
    template<typename T, typename Deleter>
    param_ref(std::unique_ptr<T, Deleter> ptr,
              typename std::enable_if<std::is_empty<Deleter>::value &&
                               std::is_default_constructible<Deleter>::value, int>::type * = nullptr)
                    : impl_(variant2::in_place_index_t<7>{},
                            std::unique_ptr<void, void(*)(void*)>(
                                static_cast<void*>(ptr.release()),
                                +[](void * ptr){delete static_cast<T*>(ptr);}),
                            typeid(T).name())
    {
    }
#endif

    /// Apply the param_ref to a statement.
    int apply(sqlite3_stmt * stmt, int c) const
    {
      return variant2::visit(visitor{stmt, c}, impl_);
    }

 private:
    struct make_visitor
    {
      template<typename T>
      auto operator()(T&& t) const -> typename std::enable_if<std::is_constructible<param_ref, T&&>::value, param_ref>::type
      {
        return param_ref(std::forward<T>(t));
      }
    };

 public:
    /// Construct param_ref from a variant
    template<typename T>
    param_ref(T && t,
            decltype(variant2::visit(make_visitor(), std::forward<T>(t))) * = nullptr)
      : param_ref(variant2::visit(make_visitor(), std::forward<T>(t)))
    {}
 private:

    struct visitor
    {
      sqlite3_stmt * stmt;
      int col;

      int operator()(variant2::monostate )
      {
        return sqlite3_bind_null(stmt, col);
      }
      int operator()(int i )
      {
        return sqlite3_bind_int(stmt, col, i);
      }
      int operator()(sqlite3_int64 i64 )
      {
        return sqlite3_bind_int64(stmt, col, i64);
      }

      int operator()(blob_view blob)
      {
        if (blob.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
          return sqlite3_bind_blob64(stmt, col, blob.data(), blob.size(), SQLITE_STATIC);
        else
          return sqlite3_bind_blob(stmt, col, blob.data(), static_cast<int>(blob.size()), SQLITE_STATIC);
      }

      int operator()(string_view text)
      {
        if (text.size() > std::numeric_limits<int>::max())
          return sqlite3_bind_text64(stmt, col, text.data(), text.size(), SQLITE_STATIC, SQLITE_UTF8);
        else
          return sqlite3_bind_text(stmt, col, text.data(), static_cast<int>(text.size()), SQLITE_STATIC);
      }
      int operator()(double value)
      {
        return sqlite3_bind_double(stmt, col, value);
      }
      int operator()(zero_blob zb)
      {
        if (static_cast<std::size_t>(zb) > static_cast<std::size_t>(std::numeric_limits<int>::max()))
          return sqlite3_bind_zeroblob64(stmt, col, static_cast<sqlite3_uint64>(zb));
        else
          return sqlite3_bind_zeroblob(stmt, col, static_cast<int>(zb));
      }
#if SQLITE_VERSION_NUMBER >= 3020000
      int operator()(std::pair<std::unique_ptr<void, void(*)(void*)>, const char*> & p)
      {
        auto d =p.first.get_deleter();
        return sqlite3_bind_pointer(stmt, col, p.first.release(), p.second, d);
      }
#endif
    };

    mutable // so we can use it with
    variant2::variant<variant2::monostate, int, sqlite3_int64,
                      blob_view, string_view, double, zero_blob
#if SQLITE_VERSION_NUMBER >= 3020000
                      , std::pair<std::unique_ptr<void, void(*)(void*)>, const char*>
#endif
                      > impl_;
};


/** @brief A statement used for a prepared-statement.
    @ingroup reference

 */
struct statement
{
    ///@{
    /** @brief execute the prepared statement once.

      @param params The arguments to be passed to the prepared statement. This can be a map or a vector of param_ref.
      @param ec     The system::error_code used to deliver errors for the exception less overload.
      @param info   The error_info used to deliver errors for the exception less overload.
      @return The resultset of the query.

      @code{.cpp}
        extern sqlite::connection conn;
        statement st = conn.prepare("select id from users where name = $1;");
        resultset q = std::move(st).execute(std::make_tuple("peter"));
      @endcode

     */
    template <typename ArgRange = std::initializer_list<param_ref>>
    resultset execute(
            ArgRange && params,
            system::error_code& ec,
            error_info& info) &&
    {
        bind_impl(std::forward<ArgRange>(params), ec, info);
        resultset rs;
        rs.impl_.reset(impl_.release());
        if (!ec)
          rs.read_next(ec, info);
        return rs;
    }

    template <typename ArgRange = std::initializer_list<param_ref>>
    resultset execute(ArgRange && params) &&
    {
        system::error_code ec;
        error_info ei;
        auto tmp = std::move(*this).execute(std::forward<ArgRange>(params), ec, ei);
        if (ec)
            detail::throw_error_code(ec, ei);
        return tmp;
    }

    resultset execute(
        std::initializer_list<std::pair<string_view, param_ref>> params,
        system::error_code& ec,
        error_info& info) &&
    {
        bind_impl(std::move(params), ec, info);
        resultset rs;
        rs.impl_.reset(impl_.release());
        if (!ec)
          rs.read_next(ec, info);
        return rs;
    }

    resultset execute(std::initializer_list<std::pair<string_view, param_ref>> params) &&
    {
      system::error_code ec;
      error_info ei;
      auto tmp = std::move(*this).execute(std::move(params), ec, ei);
      if (ec)
        detail::throw_error_code(ec, ei);
      return tmp;
    }

    template<typename T, bool Strict = false, typename ArgRange = std::initializer_list<param_ref>>
    static_resultset<T, Strict> execute(
        ArgRange && params,
        system::error_code & ec,
        error_info & ei) &&
    {
      static_resultset<T, Strict> tmp = std::move(*this).execute(std::forward<ArgRange>(params), ec, ei);
      if (ec)
        return {};
      tmp.check_columns_(ec, ei);
      if (ec)
        return {};

      return tmp;
    }

    template<typename T, bool Strict = false, typename ArgRange = std::initializer_list<param_ref>>
    static_resultset<T, Strict> execute(ArgRange && params) &&
    {
      system::error_code ec;
      error_info ei;
      auto tmp = std::move(*this).execute<T>(std::forward<ArgRange>(params), ec, ei);
      if (ec)
        throw_exception(system::system_error(ec, ei.message()));
      return tmp;
    }

    template<typename T, bool Strict = false>
    static_resultset<T, Strict> execute(
        std::initializer_list<std::pair<string_view, param_ref>> params,
        system::error_code & ec,
        error_info & ei) &&
    {
      static_resultset<T, Strict> tmp = std::move(*this).execute(std::move(params), ec, ei);
      if (ec)
        return {};
      tmp.check_columns_(ec, ei);
      if (ec)
        return {};

      return tmp;
    }

    template<typename T, bool Strict = false>
    static_resultset<T, Strict> execute(std::initializer_list<std::pair<string_view, param_ref>> params) &&
    {
      system::error_code ec;
      error_info ei;
      auto tmp = std::move(*this).execute<T, Strict>(std::move(params), ec, ei);
      if (ec)
        throw_exception(system::system_error(ec, ei.message()));
      return tmp;
    }

    ///@}

    ///@{
    /** @brief execute the prepared statement and reset it afterwards.

      @warning The handle is shared between the statement & resultset. The statemens need to be kept alive.

      @param params The arguments to be passed to the prepared statement.  This can be a map, a vector or a stuple of param_ref.
      @param ec     The system::error_code used to deliver errors for the exception less overload.
      @param info   The error_info used to deliver errors for the exception less overload.
      @return The resultset of the query.

      @code{.cpp}
        extern sqlite::connection conn;
        statement st = conn.prepare("select id from users where name = $1;");
        resultset q = std::move(st).execute(std::make_tuple("peter"));
      @endcode



     */
    template <typename ArgRange = std::initializer_list<param_ref>>
    resultset execute(
            ArgRange && params,
            system::error_code& ec,
            error_info& info) &
    {
        bind_impl(std::forward<ArgRange>(params), ec, info);
        resultset rs;
        rs.impl_.get_deleter().delete_ = false;
        rs.impl_.reset(impl_.get());
        if (!ec)
            rs.read_next(ec, info);
        return rs;
    }


    template <typename ArgRange = std::initializer_list<param_ref>>
    resultset execute(ArgRange && params) &
    {
        system::error_code ec;
        error_info ei;
        auto tmp = execute(std::forward<ArgRange>(params), ec, ei);
        if (ec)
            detail::throw_error_code(ec, ei);
        return tmp;
    }


    resultset execute(
        std::initializer_list<std::pair<string_view, param_ref>> params,
        system::error_code& ec,
        error_info& info) &
    {
      bind_impl(std::move(params), ec, info);
      resultset rs;
      rs.impl_.get_deleter().delete_ = false;
      rs.impl_.reset(impl_.get());
      if (!ec)
        rs.read_next(ec, info);
      return rs;
    }

    resultset execute(std::initializer_list<std::pair<string_view, param_ref>> params) &
    {
      system::error_code ec;
      error_info ei;
      auto tmp = execute(std::move(params), ec, ei);
      if (ec)
        detail::throw_error_code(ec, ei);
      return tmp;
    }

    template<typename T, bool Strict = false, typename ArgRange = std::initializer_list<param_ref>>
    static_resultset<T, Strict> execute(
        ArgRange && params,
        system::error_code & ec,
        error_info & ei) &
    {
      static_resultset<T, Strict> tmp = execute(std::forward<ArgRange>(params), ec, ei);
      if (ec)
        return {};
      tmp.check_columns_(ec, ei);
      if (ec)
        return {};

      return tmp;
    }

    template<typename T, bool Strict = false, typename ArgRange = std::initializer_list<param_ref>>
    static_resultset<T, Strict> execute(ArgRange && params) &
    {
      system::error_code ec;
      error_info ei;
      auto tmp = execute<T, Strict>(std::forward<ArgRange>(params), ec, ei);
      if (ec)
        throw_exception(system::system_error(ec, ei.message()));
      return tmp;
    }

    template<typename T, bool Strict = false>
    static_resultset<T, Strict> execute(
        std::initializer_list<std::pair<string_view, param_ref>> params,
        system::error_code & ec,
        error_info & ei) &
    {
      static_resultset<T, Strict> tmp = execute(std::move(params), ec, ei);
      if (ec)
        return {};
      tmp.check_columns_(ec, ei);
      if (ec)
        return {};

      return tmp;
    }

    template<typename T, bool Strict = false>
    static_resultset<T, Strict> execute(std::initializer_list<std::pair<string_view, param_ref>> params) &
    {
      system::error_code ec;
      error_info ei;
      auto tmp = execute<T, Strict>(std::move(params), ec, ei);
      if (ec)
        throw_exception(system::system_error(ec, ei.message()));
      return tmp;
    }

    ///@}


    /// Returns the sql used to construct the prepared statement.
    core::string_view sql()
    {
        return sqlite3_sql(impl_.get());
    }

#if SQLITE_VERSION_NUMBER >= 3014000
    /// Returns the expanded sql used to construct the prepared statement.
    core::string_view expanded_sql()
    {
      return sqlite3_expanded_sql(impl_.get());
    }
#endif

    /// Returns the expanded sql used to construct the prepared statement.
#ifdef SQLITE_ENABLE_NORMALIZE
    core::string_view normalized_sql()
    {
      return sqlite3_normalized_sql(impl_.get());
    }
#endif

    /// Returns the declared type of the column
    core::string_view declared_type(int id) const
    {
        return sqlite3_column_decltype(impl_.get(), id);
    }

  private:

    template<typename ... Args>
    void bind_impl(std::tuple<Args...> && vec,
                   system::error_code & ec,
                   error_info & ei)
    {
        const auto sz =  sqlite3_bind_parameter_count(impl_.get());
        if (sizeof...(Args) < static_cast<std::size_t>(sz))
        {
            BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_ERROR);
            ei.format("To few parameters provided. Needed %ld got %ld",
                      sz, sizeof...(Args));
            return;
        }

        int i = 1, ar = SQLITE_OK;
        mp11::tuple_for_each(std::move(vec),
                             [&](param_ref pr)
                             {
                                if (ar == SQLITE_OK)
                                  ar = pr.apply(impl_.get(), i++);
                             });
        if (ar != SQLITE_OK)
        {
          BOOST_SQLITE_ASSIGN_EC(ec, ar);
          ei.set_message(sqlite3_errmsg(sqlite3_db_handle(impl_.get())));
          return;
        }
    }


    template<typename ... Args>
    void bind_impl(const std::tuple<Args...> & vec,
                   system::error_code & ec,
                   error_info & ei)
    {
        const auto sz =  sqlite3_bind_parameter_count(impl_.get());
        if (static_cast<int>(sizeof...(Args)) < sz)
        {
            BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_ERROR);
            ei.format("To few parameters provided. Needed %ld got %ld",
                      sz, sizeof...(Args));
            return;
        }

        int i = 1, ar = SQLITE_OK;
        mp11::tuple_for_each(std::move(vec),
                             [&](param_ref pr)
                             {
                               if (ar == SQLITE_OK)
                                 ar = pr.apply(impl_.get(), i++);
                             });
        if (ar != SQLITE_OK)
        {
          BOOST_SQLITE_ASSIGN_EC(ec, ar);
          ei.set_message(sqlite3_errmsg(sqlite3_db_handle(impl_.get())));
          return;
        }
    }

    template<typename ParamVector>
    void bind_impl(ParamVector && vec, system::error_code & ec, error_info & ei,
                   typename std::enable_if<std::is_convertible<
                       typename std::decay<ParamVector>::type::value_type, param_ref>::value>::type * = nullptr)
    {
        const auto sz =  sqlite3_bind_parameter_count(impl_.get());
        if (vec.size() < static_cast<std::size_t>(sz))
        {
            BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_ERROR);
            ei.format("To few parameters provided. Needed %ld got %ld",
                      sz, vec.size());
        }
        int i = 1;
        for (const param_ref & pr : std::forward<ParamVector>(vec))
        {
          int ar = pr.apply(impl_.get(), i++);
          if (ar != SQLITE_OK)
          {

            BOOST_SQLITE_ASSIGN_EC(ec, ar);
            ei.set_message(sqlite3_errmsg(sqlite3_db_handle(impl_.get())));
            return;
          }
        }
    }

    template<typename ParamMap>
    void bind_impl(ParamMap && vec, system::error_code & ec, error_info & ei,
                   typename std::enable_if<
                       std::is_convertible<typename std::decay<ParamMap>::type::key_type, string_view>::value &&
                       std::is_convertible<typename std::decay<ParamMap>::type::mapped_type, param_ref>::value
                         >::type * = nullptr)
    {
        for (auto i = 1; i <= sqlite3_bind_parameter_count(impl_.get()); i ++)
        {
          auto c = sqlite3_bind_parameter_name(impl_.get(), i);
          if (c == nullptr)
          {
              BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_MISUSE);
              ei.set_message("Parameter maps require all parameters to be named.");
              return ;
          }
          auto itr = vec.find(c+1);
          if (itr == vec.end())
          {
            BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_MISUSE);
            ei.format("Can't find value for key '%s'", c+1);
            return ;
          }
          int ar = SQLITE_OK;
          if (std::is_rvalue_reference<ParamMap&&>::value)
            ar = param_ref(std::move(itr->second)).apply(impl_.get(), i);
          else
            ar = param_ref(itr->second).apply(impl_.get(), i);

          if (ar != SQLITE_OK)
          {

            BOOST_SQLITE_ASSIGN_EC(ec, ar);
            ei.set_message(sqlite3_errmsg(sqlite3_db_handle(impl_.get())));
            return;
          }
        }
    }

    void bind_impl(std::initializer_list<std::pair<string_view, param_ref>> params,
                   system::error_code & ec, error_info & ei)
    {
        for (auto i = 1; i <= sqlite3_bind_parameter_count(impl_.get()); i ++)
        {
          auto c = sqlite3_bind_parameter_name(impl_.get(), i);
          if (c == nullptr)
          {
              BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_MISUSE);
              ei.set_message("Parameter maps require all parameters to be named.");
              return ;
          }

          auto itr = std::find_if(params.begin(), params.end(),
                                  [&](const std::pair<string_view, param_ref> & p)
                                  {
                                      return p.first == (c+1);
                                  });
          if (itr == params.end())
          {
            BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_MISUSE);
            ei.format("Can't find value for key '%s'", c+1);
            return ;
          }
          auto ar = itr->second.apply(impl_.get(), i);
          if (ar != SQLITE_OK)
          {

            BOOST_SQLITE_ASSIGN_EC(ec, ar);
            ei.set_message(sqlite3_errmsg(sqlite3_db_handle(impl_.get())));
            return;
          }
        }
    }


    friend
    struct connection;
    struct deleter_
    {
        void operator()(sqlite3_stmt * sm)
        {
            sqlite3_finalize(sm);
        }
    };
    std::unique_ptr<sqlite3_stmt, deleter_> impl_;
};

BOOST_SQLITE_END_NAMESPACE

#endif //BOOST_SQLITE_STATEMENT_HPP