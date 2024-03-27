//
// Copyright (c) 2024 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_STATIC_RESULTSET_HPP
#define BOOST_SQLITE_STATIC_RESULTSET_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/resultset.hpp>
#include <boost/sqlite/connection.hpp>

#include <boost/describe/members.hpp>


#if __cplusplus >= 202002L
#include <boost/pfr/core.hpp>
#include <boost/pfr/core_name.hpp>
#include <boost/pfr/traits.hpp>
#endif

BOOST_SQLITE_BEGIN_NAMESPACE

namespace detail
{

template<typename T>
struct value_to_tag {};

inline value        tag_invoke(value_to_tag<value>,        const field & f) {return f.get_value();}
inline int          tag_invoke(value_to_tag<int>,          const field & f) {return f.get_int();}
inline sqlite_int64 tag_invoke(value_to_tag<sqlite_int64>, const field & f) {return f.get_int64();}
inline double       tag_invoke(value_to_tag<double>,       const field & f) {return f.get_double();}

template<typename Allocator, typename Traits>
inline std::basic_string<char, Allocator, Traits>
                    tag_invoke(value_to_tag<std::basic_string<char, Allocator, Traits>>, const field & f)
{
  return f.get_text();
}

inline string_view  tag_invoke(value_to_tag<string_view>, const field & f) {return f.get_text();}
inline blob         tag_invoke(value_to_tag<blob>,        const field & f) {return blob(f.get_blob());}
inline blob_view    tag_invoke(value_to_tag<blob_view>,   const field & f) {return f.get_blob();}


template<typename>
struct check_columns_tag {};


template<typename>
struct convert_row_tag {};

template<typename ... Args>
void tag_invoke(check_columns_tag<std::tuple<Args...>>, const resultset & r,
                system::error_code &ec, error_info & ei)
{
  if (r.column_count() != sizeof...(Args))
  {
    BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_MISMATCH);
    ei.format("Tuple size doesn't match column count [%d != %d]", sizeof...(Args), r.column_count());
  }
}

template<typename Tuple, std::size_t ... Ns>
Tuple convert_row_to_tuple_impl(convert_row_tag<Tuple>, const row & r,
                                              mp11::index_sequence<Ns...>)
{
  return { tag_invoke(value_to_tag<typename std::tuple_element<Ns, Tuple>::type>{}, r[Ns])... };

}


template<typename ... Args>
std::tuple<Args...> tag_invoke(convert_row_tag<std::tuple<Args...>> tag, const row & r)
{
  return convert_row_to_tuple_impl(tag, r, mp11::make_index_sequence<sizeof...(Args)>{});
}


template<typename T, typename = typename std::enable_if<describe::has_describe_members<T>::value>::type>
void tag_invoke(check_columns_tag<T>, const resultset & r,
                system::error_code &ec, error_info & ei)
{
  using mems = boost::describe::describe_members<T, describe::mod_public>;
  constexpr std::size_t sz = mp11::mp_size<mems>();
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
          ei.format("Described field %Q not found in resultset struct.", d.name);
        });
  }
}

template<typename T, typename = typename std::enable_if<describe::has_describe_members<T>::value>::type>
T tag_invoke(convert_row_tag<T> tag, const row & r)
{
  T res;
  for (auto && c: r)
  {
    boost::mp11::mp_for_each<boost::describe::describe_members<T, describe::mod_public> >(
        [&](auto D)
        {
          if (D.name == c.column_name())
          {
            auto & r = res.*D.pointer;
            r = tag_invoke(value_to_tag<std::decay_t<decltype(r)>>{}, c);
          }
        });
  }
  return res;
}


#if __cplusplus >= 202002L

template<typename T>
  requires (std::is_aggregate_v<T> && !describe::has_describe_members<T>::value)
void tag_invoke(check_columns_tag<T>, const resultset & r,
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
          ei.format("PFR field %Q not found in resultset struct.", pfr::get_name<sz, T>() );
        });
  }
}

template<typename T>
    requires (std::is_aggregate_v<T> && !describe::has_describe_members<T>::value)
T tag_invoke(convert_row_tag<T> tag, const row & r)
{
  T res;
  for (auto && c: r)
  {
    boost::mp11::mp_for_each<mp11::mp_iota_c<pfr::tuple_size_v<T>>>(
        [&](auto D)
        {

          if (pfr::get_name<D, T>() == c.column_name().c_str())
          {
            auto & r = pfr::get<D()>(res);
            r = tag_invoke(value_to_tag<std::decay_t<decltype(r)>>{}, c);
          }
        });
  }
  return res;
}

#endif

}

/**
  @brief A typed resultset using a tuple or a described struct.
  @ingroup reference
  @tparam T The static type of the query.


  If is a forward-range with output iterators.

  @par Example

  @code{.cpp}

  extern sqlite::connection conn;
  struct user { std::string first_name; std::string last_name; };
  BOOST_DESCRIBE_STRUCT(user, (), (first_name, last_name));

  sqlite::resultset rs = conn.query("select first_name, last_name from users;");

  do
  {
    user usr = r.current();
    handle_row(u);
  }
  while (rs.read_next()) // read it line by line

  @endcode

*/
template<typename T>
struct static_resultset
{
  /// Returns the current row.
  T current() const &
  {
    return detail::tag_invoke(detail::convert_row_tag<T>{}, result_.current());
  }
  /// Checks if the last row has been reached.
  bool done() const {return result_.done();}

  ///@{
  /// Read the next row. Returns false if there's nothing more to read.
  BOOST_SQLITE_DECL bool read_next(system::error_code & ec, error_info & ei) { return result_.read_next(); }
  BOOST_SQLITE_DECL bool read_next()                                         { return result_.read_next(); }
  ///@}

  ///
  std::size_t column_count() const { return result_.column_count(); }
  /// Returns the name of the column idx.
  core::string_view column_name(std::size_t idx) const { return result_.column_name(idx); }

  /// Returns the name of the source table for column idx.
  core::string_view table_name(std::size_t idx) const { return result_.table_name(idx);}
  /// Returns the origin name of the column for column idx.
  core::string_view column_origin_name(std::size_t idx) const  { return result_.column_origin_name(idx);}

  static_resultset() = default;
  static_resultset(resultset && result) : result_(std::move(result))
  {
  }


  /// The input iterator can be used to read every row in a for-loop
  struct iterator
  {
    using value_type = T;
    using difference_type   = int;
    using reference         = T&;
    using iterator_category = std::forward_iterator_tag;

    iterator()
    {

    }
    explicit iterator(resultset::iterator itr) : itr_(itr)
    {
      if (itr->size() > 0ul)
        value_ = detail::tag_invoke(detail::convert_row_tag<T>{}, *itr);
    }

    bool operator!=(iterator rhs) const
    {
      return itr_ != rhs.itr_;
    }

    value_type &operator*()  { return value_; }
    value_type *operator->() { return &value_; }

    iterator& operator++()
    {
      ++itr_;
      if (itr_->size() > 0ul)
        value_ = detail::tag_invoke(detail::convert_row_tag<T>{}, *itr_);
      return *this;
    }
    iterator operator++(int)
    {
      auto l = *this;
      ++(*this);
      return l;
    }
   private:
    resultset::iterator itr_;
    value_type value_;
  };

  /// Return an input iterator to the currently unread row
  iterator begin() { return iterator(result_.begin());}
  /// Sentinel iterator.
  iterator   end() { return iterator(result_.end()); }


 private:
  friend struct connection;
  friend struct statement;
  resultset result_;
  void check_columns_( system::error_code & ec, error_info & ei)
  {
    detail::tag_invoke(detail::check_columns_tag<T>{}, result_, ec, ei);
  }
};

BOOST_SQLITE_END_NAMESPACE

#endif //BOOST_SQLITE_STATIC_RESULTSET_HPP
