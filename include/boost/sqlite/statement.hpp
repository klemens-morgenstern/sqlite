// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_STATEMENT_HPP
#define BOOST_SQLITE_STATEMENT_HPP

#include <tuple>
#include <boost/mp11/algorithm.hpp>
#include <boost/core/ignore_unused.hpp>

namespace boost {
namespace sqlite {


namespace detail {

struct binder
{
    sqlite3_stmt * stm_;
    int col;

    int operator()(variant2::monostate)
    {
      return sqlite3_bind_null(stm_, col);
    }
    int operator()(std::nullptr_t)
    {
        return sqlite3_bind_null(stm_, col);
    }
    template<typename I,
            typename = std::enable_if_t<std::is_integral<I>::value>>
    int operator()(I value)
    {
        BOOST_IF_CONSTEXPR ((sizeof(I) == sizeof(int) && std::is_unsigned<I>::value)
            || (sizeof(I) > sizeof(int)))
            return sqlite3_bind_int64(stm_, col, static_cast<sqlite3_int64>(value));
        else
            return sqlite3_bind_int(stm_, col, static_cast<int>(value));
    }
    int operator()(blob_view blob)
    {
        if (blob.size() > std::numeric_limits<int>::max())
            return sqlite3_bind_blob(stm_, col, blob.data(), blob.size(), nullptr);
        else
            return sqlite3_bind_blob64(stm_, col, blob.data(), blob.size(), nullptr);
    }

    int operator()(string_view text)
    {
        if (text.size() > std::numeric_limits<int>::max())
            return sqlite3_bind_text(stm_, col, text.data(), text.size(), nullptr);
        else
            return sqlite3_bind_text64(stm_, col, text.data(), text.size(), nullptr, SQLITE_UTF8);
    }

    int operator()(double value)
    {
        return sqlite3_bind_double(stm_, col, value);
    }

    template<typename T>
    int operator()(std::unique_ptr<T> ptr)
    {
      return sqlite3_bind_pointer(stm_, col, ptr.release(), typeid(T).name(),
                                  +[](void * ptr){delete static_cast<T*>(ptr);});
    }

    template<typename T>
    int operator()(std::unique_ptr<T, void(*)(T*)> ptr)
    {
      return sqlite3_bind_pointer(stm_, col, ptr.release(), typeid(T).name(),
                                  static_cast<void(*)(void*)>(ptr.get_deleter()));
    }

    template<typename T, typename Deleter>
    auto operator()(std::unique_ptr<T, Deleter> ptr)
        -> typename std::enable_if<std::is_empty<Deleter>::value &&
                                   std::is_default_constructible<Deleter>::value, int>::type
    {
      return sqlite3_bind_pointer(stm_, col, ptr.release(), typeid(T).name(),
                                  +[](void * ptr){Deleter()(static_cast<T*>(ptr));});
    }
};

}


/** @brief A statement used for a prepared-statement.
    @ingroup reference

 */
struct statement
{
    ///@{
    /** @brief execute the prepared statement once.

      @param params The arguments to be passed to the prepared statement.
      @param ec     The error_code used to deliver errors for the exception less overload.
      @param info   The error_info used to deliver errors for the exception less overload.
      @return The resultset of the query.

      @code{.cpp}
        extern sqlite::connection conn;
        statement st = conn.prepare("select id from users where name = $1;");
        resultset q = std::move(st).execute(std::make_tuple("peter"));
      @endcode

     */
    template <typename ... Args>
    resultset execute(
            const std::tuple<Args...>& params,
            error_code& ec,
            error_info& info) &&
    {
        bind_impl(params, ec, info, std::make_index_sequence<sizeof...(Args)>{});
        resultset rs;
        rs.impl_.reset(impl_.release());
        return rs;
    }


    template <typename ... Args>
    resultset execute(const std::tuple<Args...>& params) &&
    {
        system::error_code ec;
        error_info ei;
        auto tmp = std::move(*this).execute(params, ec, ei);
        if (ec)
            throw_exception(system::system_error(ec, ei.message()));
        return tmp;
    }

    template <typename ... Args>
    resultset execute(
            std::tuple<Args...>&& params,
            error_code& ec,
            error_info& info) &&
    {
        bind_impl(std::move(params), ec, info, std::make_index_sequence<sizeof...(Args)>{});
        resultset rs;
        rs.impl_.reset(impl_.release());
        return rs;
    }


    template <typename ... Args>
    resultset execute(std::tuple<Args...>&& params) &&
    {
        system::error_code ec;
        error_info ei;
        auto tmp = std::move(*this).execute(std::move(params), ec, ei);
        if (ec)
            throw_exception(system::system_error(ec, ei.message()));
        return tmp;
    }
    ///@}

    ///@{
    /** @brief execute the prepared statement and reset it afterwards.

      @warning The handle is shared between the statement & resultset. The statement need to be keapt alive

      @param params The arguments to be passed to the prepared statement.
      @param ec     The error_code used to deliver errors for the exception less overload.
      @param info   The error_info used to deliver errors for the exception less overload.
      @return The resultset of the query.

      @code{.cpp}
        extern sqlite::connection conn;
        statement st = conn.prepare("select id from users where name = $1;");
        resultset q = std::move(st).execute(std::make_tuple("peter"));
      @endcode

     */
    template <typename ... Args>
    resultset execute(
            const std::tuple<Args...>& params,
            error_code& ec,
            error_info& info) &
    {
        bind_impl(params, ec, info, std::make_index_sequence<sizeof...(Args)>{});
        resultset rs;
        rs.impl_.get_deleter().delete_ = false;
        rs.impl_.reset(impl_.get());
        return rs;
    }


    template <typename ... Args>
    resultset execute(const std::tuple<Args...>& params) &
    {
        system::error_code ec;
        error_info ei;
        auto tmp = execute(params, ec, ei);
        if (ec)
            throw_exception(system::system_error(ec, ei.message()));
        return tmp;
    }
    template <typename ... Args>
    resultset execute(
            std::tuple<Args...>&& params,
            error_code& ec,
            error_info& info) &
    {
        bind_impl(std::move(params), ec, info, std::make_index_sequence<sizeof...(Args)>{});
        resultset rs;
        rs.impl_.get_deleter().delete_ = false;
        rs.impl_.reset(impl_.get());
        return rs;
    }


    template <typename ... Args>
    resultset execute(std::tuple<Args...>&& params) &
    {
        system::error_code ec;
        error_info ei;
        auto tmp = execute(std::move(params), ec, ei);
        if (ec)
            throw_exception(system::system_error(ec, ei.message()));
        return tmp;
    }
    ///@}


    /// Get the sql used to construct the prepared statement.
    core::string_view sql()
    {
        return sqlite3_sql(impl_.get());
    }

  private:

    template<typename T>
    bool bind_step(
            T && value,
            std::size_t idx,
            error_code & ec,
            error_info & ei)
    {
        if (!ec)
        {
            auto cc = detail::binder{impl_.get(), static_cast<int>(idx)}(std::forward<T>(value));
            if (cc != SQLITE_OK)
            {
                BOOST_SQLITE_ASSIGN_EC(ec, cc);
                ei.set_message(sqlite3_errmsg(sqlite3_db_handle(impl_.get())));
            }
        }
        return !ec;
    }

    template<typename ... Args, std::size_t ... Idx>
    void bind_impl(std::tuple<Args...> && tupl,
                   error_code & ec,
                   error_info & ei,
                   std::index_sequence<Idx...>)
    {
        const auto sz =  sqlite3_bind_parameter_count(impl_.get());
        if (sizeof...(Idx) < sz)
        {
          BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_ERROR);
          ei.set_message("To few parameters provided. Needs " + std::to_string(sz)
                       + " got " + std::to_string(sizeof...(Idx)));
        }

        boost::ignore_unused(bind_step(std::move(std::get<Idx>(tupl)), Idx + 1, ec, ei)...);
    }


    template<typename ... Args, std::size_t ... Idx>
    void bind_impl(const std::tuple<Args...> & tupl,
                   error_code & ec,
                   error_info & ei,
                   std::index_sequence<Idx...>)
    {
      const auto sz =  sqlite3_bind_parameter_count(impl_.get());
      if (sizeof...(Idx) < sz)
      {
        BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_ERROR);
        ei.set_message("To few parameters provided. Needs " + std::to_string(sz)
                       + " got " + std::to_string(sizeof...(Idx)));
      }

      boost::ignore_unused(bind_step(std::get<Idx>(tupl), Idx + 1, ec, ei)...);
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

}
}

#endif //BOOST_SQLITE_STATEMENT_HPP