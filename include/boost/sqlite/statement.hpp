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

    int operator()(core::string_view text)
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
};


}

struct statement
{
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

    core::string_view sql()
    {
        return sqlite3_sql(impl_.get());
    }

  private:

    template<std::size_t Idx, typename Tuple>
    bool bind_step(
            Tuple & tupl,
            error_code & ec,
            error_info & ei)
    {
        if (!ec)
        {
            auto cc = detail::binder{impl_.get(), Idx + 1}(std::get<Idx>(tupl));
            if (cc != SQLITE_OK)
            {
                BOOST_SQLITE_ASSIGN_EC(ec, cc);
                ei.set_message(sqlite3_errmsg(sqlite3_db_handle(impl_.get())));
            }
        }
        return !ec;
    }



    template<typename Tuple, std::size_t ... Idx>
    void bind_impl(Tuple tupl,
                   error_code & ec,
                   error_info & ei,
                   std::index_sequence<Idx...>)
    {
        boost::ignore_unused(bind_step<Idx>(tupl,ec, ei)...);
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