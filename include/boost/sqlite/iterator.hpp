//
// Copyright (c) 2025 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_ITERATOR_HPP
#define BOOST_SQLITE_ITERATOR_HPP


#include <boost/describe/members.hpp>

#include <array>
#include <cstdint>

#if __cplusplus >= 202002L
#include <boost/pfr/core.hpp>
#include <boost/pfr/core_name.hpp>
#include <boost/pfr/traits.hpp>
#endif


namespace boost { template<typename> class optional;}

#if __cplusplus >= 201702L
#include  <optional>
#endif

#include <boost/sqlite/statement.hpp>
#include <boost/sqlite/error.hpp>


BOOST_SQLITE_BEGIN_NAMESPACE

namespace detail
{

    inline void convert_field(sqlite_int64 & target, const field & f) {target = f.get_int();}

    template<typename = std::enable_if_t<!std::is_same<std::int64_t, sqlite_int64>::value>>
    inline void convert_field(std::int64_t & target, const field & f)
    {
        target = static_cast<std::int64_t>(f.get_int());
    }

    inline void convert_field(double & target, const field & f) {target = f.get_double();}


    template<typename Traits = std::char_traits<char>, typename Allocator = std::allocator<char>>
    inline void convert_field(std::basic_string<char, Traits, Allocator> & target, const field & f)
    {
        auto t = f.get_text();
        target.assign(t.begin(), t.end());
    }

    inline void convert_field(string_view & target, const field & f) {target = f.get_text();}
    inline void convert_field(blob & target,        const field & f) {target = blob(f.get_blob());}
    inline void convert_field(blob_view & target,   const field & f) {target = f.get_blob();}

    #if __cplusplus >= 201702L
    template<typename T>
    inline void convert_field(std::optional<T> & target, const field & f)
    {
        if (f.is_null())
            target.reset();
        else
            convert_field(target.emplace(), f);
    }
    #endif

    template<typename T>
    inline void convert_field(boost::optional<T> & target, const field & f)
    {
        if (f.is_null())
            target.reset();
        else
            return convert_field(target.emplace_back(), f);
    }

    template<typename T>
    inline constexpr bool field_type_is_nullable(const T& ) {return false;}
    #if __cplusplus >= 201702L
    template<typename T>
    inline bool field_type_is_nullable(const std::optional<T> &) { return true; }
    #endif
    template<typename T>
    inline bool field_type_is_nullable(const boost::optional<T> &) { return true; }

    inline value_type required_field_type(const sqlite3_int64 &) {return value_type::integer;}

    template<typename = std::enable_if_t<!std::is_same<std::int64_t, sqlite_int64>::value>>
    inline value_type required_field_type(const std::int64_t &) {return value_type::integer;}

    template<typename Allocator, typename Traits>
    inline value_type required_field_type(const std::basic_string<char, Allocator, Traits> & )
    {
        return value_type::text;
    }

    inline value_type required_field_type(const string_view &) {return value_type::text;}
    inline value_type required_field_type(const blob &)        {return value_type::blob;}
    inline value_type required_field_type(const blob_view &)   {return value_type::blob;}


    inline void check_columns(const row *, const statement& r, system::error_code &, error_info & )
    {

    }


    template<typename ... Args>
    void check_columns(const std::tuple<Args...> *, const statement & r,
                       system::error_code &ec, error_info & ei)
    {
        if (r.column_count() != sizeof...(Args))
        {
            BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_MISMATCH);
            ei.format("Tuple size doesn't match column count [%ld != %ld]", sizeof...(Args), r.column_count());
        }
    }

    template<bool Strict>
    void convert_row(row & res, const row & r, system::error_code , error_info & ) {res = r;}


    template<bool Strict, typename ... Args>
    void convert_row(std::tuple<Args...> & res, const row & r, system::error_code ec, error_info & ei)
    {
        std::size_t idx = 0u;

        mp11::tuple_for_each(
            res,
            [&](auto & v)
            {
                const auto i = idx++;
                const auto & f = r[i];
                BOOST_IF_CONSTEXPR (Strict)
                {
                    if (!ec) // only check if we don't have an error yet.
                    {
                        if (f.is_null() && !field_type_is_nullable(v))
                        {
                            BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_CONSTRAINT_NOTNULL);
                            ei.format("unexpected null in column %d", i);
                        }
                        else if (f.type() != required_field_type(v))
                        {
                            BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_CONSTRAINT_DATATYPE);
                            ei.format("unexpected type [%s] in column %d, expected [%s]",
                                      value_type_name(f.type()), i, value_type_name(required_field_type(v)));
                        }
                    }
                }
                else
                    boost::ignore_unused(ec, ei);

                detail::convert_field(v, f);
            });
    }

    #if defined(BOOST_DESCRIBE_CXX14)

    template<typename T, typename = typename std::enable_if<describe::has_describe_members<T>::value>::type>
    void check_columns(const T *, const statement & r,
                       system::error_code &ec, error_info & ei)
    {
        using mems = boost::describe::describe_members<T, describe::mod_public>;
        constexpr std::size_t sz = mp11::mp_size<mems>();
        if (r.column_count() != sz)
        {
            BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_MISMATCH);
            ei.format("Describe size doesn't match column count [%ld != %ld]", sz, r.column_count());
        }

        // columns can be duplicated!
        std::array<bool, sz> found;
        std::fill(found.begin(), found.end(), false);

        for (std::size_t i = 0ul; i < r.column_count(); i++)
        {
            bool cfound = false;
            boost::mp11::mp_for_each<mp11::mp_iota_c<sz>>(
                [&](auto sz)
                {
                    auto d = mp11::mp_at_c<mems, sz>();
                    if (d.name == r.column_name(i))
                    {
                        found[sz] = true;
                        cfound = true;
                    }
                });

            if (!cfound)
            {
                BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_MISMATCH);
                ei.format("Column %Q not found in described struct.", r.column_name(i));
                break;
            }
        }

        if (ec)
            return;


        auto itr = std::find(found.begin(), found.end(), false);
        if (itr != found.end())
        {
            mp11::mp_with_index<sz>(
                std::distance(found.begin(), itr),
                                    [&](auto sz)
                                    {
                                        auto d = mp11::mp_at_c<mems, sz>();
                                        BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_MISMATCH);
                                        ei.format("Described field %Q not found in statement struct.", d.name);
                                    });
        }
    }

    template<bool Strict, typename T,
    typename = typename std::enable_if<describe::has_describe_members<T>::value>::type>
    void convert_row(T & res, const row & r, system::error_code ec, error_info & ei)
    {
        for (auto && f: r)
        {
            boost::mp11::mp_for_each<boost::describe::describe_members<T, describe::mod_public> >(
                [&](auto D)
                {
                    if (D.name == f.column_name())
                    {
                        auto & r = res.*D.pointer;
                        BOOST_IF_CONSTEXPR(Strict)
                        {
                            if (!ec)
                            {
                                if (f.is_null() && !field_type_is_nullable(r))
                                {
                                    BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_CONSTRAINT_NOTNULL);
                                    ei.format("unexpected null in column %s", D.name);
                                }
                                else if (f.type() != required_field_type(r))
                                {
                                    BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_CONSTRAINT_DATATYPE);
                                    ei.format("unexpected type [%s] in column %s, expected [%s]",
                                              value_type_name(f.type()), D.name, value_type_name(required_field_type(r)));
                                }
                            }
                        }

                        detail::convert_field(r, f);
                    }
                });
        }
    }

    #endif

    #if __cplusplus >= 202002L

    template<typename T>
    requires (std::is_aggregate_v<T> && !describe::has_describe_members<T>::value)
    void check_columns(const T *, const statement & r,
                       system::error_code &ec, error_info & ei)
    {
        constexpr std::size_t sz = pfr::tuple_size_v<T>;
        if (r.column_count() != sz)
        {
            BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_MISMATCH);
            ei.format("Describe size doesn't match column count [%d != %d]", sz, r.column_count());
        }

        // columns can be duplicated!
        std::array<bool, sz> found;
        std::fill(found.begin(), found.end(), false);

        for (std::size_t i = 0ul; i < r.column_count(); i++)
        {
            bool cfound = false;
            boost::mp11::mp_for_each<mp11::mp_iota_c<sz>>(
                [&](auto sz)
                {
                    if (pfr::get_name<sz, T>() == r.column_name(i))
                    {
                        found[sz] = true;
                        cfound = true;
                    }
                });

            if (!cfound)
            {
                BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_MISMATCH);
                ei.format("Column %Q not found in  struct.", r.column_name(i));
                break;
            }
        }

        if (ec)
            return;


        auto itr = std::find(found.begin(), found.end(), false);
        if (itr != found.end())
        {
            mp11::mp_with_index<sz>(
                std::distance(found.begin(), itr),
                                    [&](auto sz)
                                    {
                                        BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_MISMATCH);
                                        ei.format("PFR field %Q not found in statement struct.", pfr::get_name<sz, T>() );
                                    });
        }
    }

    template<bool Strict, typename T>
    requires (std::is_aggregate_v<T> && !describe::has_describe_members<T>::value)
    void convert_row(T & res, const row & r, system::error_code ec, error_info & ei)
    {
        for (auto && f: r)
        {
            boost::mp11::mp_for_each<mp11::mp_iota_c<pfr::tuple_size_v<T>>>(
                [&](auto D)
                {
                    if (pfr::get_name<D, T>() == f.column_name().c_str())
                    {
                        auto & r = pfr::get<D()>(res);
                        BOOST_IF_CONSTEXPR(Strict)
                        {
                            if (!ec)
                            {
                                if (f.is_null() && !field_type_is_nullable(r))
                                {
                                    BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_CONSTRAINT_NOTNULL);
                                    ei.format("unexpected null in column %s", D.name);
                                }
                                else if (f.type() != required_field_type(r))
                                {
                                    BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_CONSTRAINT_DATATYPE);
                                    ei.format("unexpected type [%s] in column %s, expected [%s]",
                                              value_type_name(f.type()), D.name, value_type_name(required_field_type(r)));
                                }
                            }
                        }
                        detail::convert_field(r, f);
                    }
                });
        }
    }

    #endif

}


template<typename T = row, bool Strict = false>
struct statement_iterator
{
    using value_type = T;
    using reference = T&;
    using iterator_category = std::input_iterator_tag;

    statement_iterator() = default;
    statement_iterator(statement & st) : st_(&st)
    {
       st_->step();
       if (st_->done())
           st_ = nullptr;
       else
       {
           system::error_code ec;
           error_info ei;
           detail::check_columns(static_cast<T*>(nullptr), *st_, ec, ei);
           if (!ec)
             detail::convert_row<Strict >(row_, st_->current(), ec, ei);
           if (ec)
             handle_error_(ec, ei);
        }


    }

    bool operator!=(statement_iterator rhs) const
    {
        return st_!= rhs.st_;
    }

    T &operator*()  { return row_; }
    T *operator->() { return &row_; }

    statement_iterator operator++()
    {
        st_->step();
        if (st_->done())
            st_ = nullptr;
        else
        {
            system::error_code ec;
            error_info ei;
            detail::convert_row<Strict >(row_, st_->current(), ec, ei);
            if (ec)
                handle_error_(ec, ei);
        }
        return *this;
    }

 private:
    statement * st_ = nullptr;
    T row_;

    void handle_error_(system::error_code &ec, error_info & ei)
    {
        handle_error_impl_(is_result_type<T>{}, ec, ei);
    }
    void handle_error_impl_(std::true_type  /* is_result_type */, system::error_code &ec, error_info & ei)
    {
        row_ = T(boost::system::in_place_error, error{ec, std::move(ei)});
    }
    BOOST_NORETURN
    void handle_error_impl_(std::false_type /* is_result_type */, system::error_code &ec, error_info & ei)
    {
        throw_exception(system::system_error(ec, ei.message()));
    }
};


template<typename T = row, bool Strict = false>
struct statement_range
{
    statement_range(statement & st) : st_(st) {}
    statement_iterator<T, Strict> begin() const {return {st_};}
    statement_iterator<T, Strict> end()   const {return {};}
private:
    statement & st_;
};


BOOST_SQLITE_END_NAMESPACE


#endif // BOOST_SQLITE_ITERATOR_HPP
