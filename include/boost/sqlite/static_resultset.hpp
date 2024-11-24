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


template<typename ... Args>
void check_columns(const std::tuple<Args...> *, const resultset & r,
                   system::error_code &ec, error_info & ei)
{
  if (r.column_count() != sizeof...(Args))
  {
    BOOST_SQLITE_ASSIGN_EC(ec, SQLITE_MISMATCH);
    ei.format("Tuple size doesn't match column count [%d != %d]", sizeof...(Args), r.column_count());
  }
}

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
void check_columns(const T *, const resultset & r,
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
void check_columns(const T *, const resultset & r,
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

/**
  @brief A typed resultset using a tuple or a described struct.
  @ingroup reference
  @tparam T The static type of the query.
  @tparam Strict Disables implicit conversions.

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
template<typename T, bool Strict >
struct static_resultset
{
  /// Returns the current row.
  T current() const &
  {
    system::error_code ec;
    error_info ei;
    auto tmp = current(ec, ei);
    if (ec)
      throw_exception(system::system_error(ec, ei.message()));
    return tmp;
  }

  /// Returns the current row.
  T current(system::error_code & ec, error_info & ei) const &
  {
    T res;
    detail::convert_row<Strict>(res, result_.current(), ec, ei);
    return res;
  }

  /// Checks if the last row has been reached.
  bool done() const {return result_.done();}

  ///@{
  /// Read the next row. Returns false if there's nothing more to read.
  BOOST_SQLITE_DECL bool read_next(system::error_code & ec, error_info & ei) { return result_.read_next(ec, ei); }
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
  static_resultset(resultset && result) : result_(std::move(result)) { }


  static_resultset(static_resultset<T, false> && rhs) : result_(std::move(rhs.result_)) {}

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
      system::error_code ec;
      error_info ei;
      if (itr->size() > 0ul)
        detail::convert_row<Strict>(value_, *itr, ec, ei);
      if (ec)
        throw_exception(system::system_error(ec, ei.message()));
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

      system::error_code ec;
      error_info ei;
      if (itr_->size() > 0ul)
        detail::convert_row<Strict>(value_, *itr_, ec, ei);
      if (ec)
        throw_exception(system::system_error(ec, ei.message()));

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



  static_resultset<T, true> strict() &&
  {
    return {std::move(result_)};
  }
 private:

  friend struct static_resultset<T, true>;
  friend struct connection;
  friend struct statement;
  resultset result_;
  void check_columns_( system::error_code & ec, error_info & ei)
  {
    detail::check_columns(static_cast<T*>(nullptr), result_, ec, ei);
  }
};

BOOST_SQLITE_END_NAMESPACE

#endif //BOOST_SQLITE_STATIC_RESULTSET_HPP
