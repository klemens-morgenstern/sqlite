// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
// based on boost.process
#ifndef BOOST_SQLITE_CSTRING_REF_HPP
#define BOOST_SQLITE_CSTRING_REF_HPP

#include <boost/config.hpp>
#include <boost/core/detail/string_view.hpp>

#include <cstddef>
#include <type_traits>
#include <string>

namespace boost
{
namespace sqlite
{

/// Small wrapper for a null-terminated string that can be directly passed to C APIS
/** This ref can only be modified by moving the front pointer. It does not store the
 * size, but can detect values that can directly be passed to system APIs.
 *
 * It can be constructed from a `char*` pointer or any class that has a `c_str()`
 * member function, e.g. std::string or boost::static_string.
 *
 */
struct cstring_ref
{
  using value_type             = char;
  using traits_type            = std::char_traits<char>;
  BOOST_CONSTEXPR cstring_ref() noexcept : view_("") {}
  BOOST_CONSTEXPR cstring_ref(std::nullptr_t) = delete;

  BOOST_CONSTEXPR cstring_ref( const value_type* s ) : view_(s) {}

  template<typename Source,
      typename =
      typename std::enable_if<
          std::is_same<const value_type,
              typename std::remove_pointer<decltype(std::declval<Source>().c_str())>::type
          >::value>::type>
  BOOST_CONSTEXPR cstring_ref(Source && src) : view_(src.c_str()) {}

  BOOST_CONSTEXPR const char * c_str() const BOOST_NOEXCEPT
  {
    return this->data();
  }

  using string_view_type = core::string_view;
  constexpr operator string_view_type() const {return view_;}

  using pointer                =       char *;
  using const_pointer          = const char *;
  using reference              =       char &;
  using const_reference        = const char &;
  using const_iterator         = const_pointer;
  using iterator               = const_iterator;
  using const_reverse_iterator = typename std::reverse_iterator<const_iterator>;
  using reverse_iterator       = typename std::reverse_iterator<iterator>;
  using size_type              = std::size_t;
  using difference_type        = std::ptrdiff_t;

  static BOOST_CONSTEXPR size_type npos = -1;

  BOOST_CONSTEXPR const_iterator begin()  const BOOST_NOEXCEPT {return view_;};
  BOOST_CONSTEXPR const_iterator end()    const BOOST_NOEXCEPT {return view_ + length();};
  BOOST_CONSTEXPR const_iterator cbegin() const BOOST_NOEXCEPT {return view_;};
  BOOST_CONSTEXPR const_iterator cend()   const BOOST_NOEXCEPT {return view_ + length();};

#if defined(BOOST_NO_CXX17)
  const_reverse_iterator rbegin()  const BOOST_NOEXCEPT {return reverse_iterator(end());};
  const_reverse_iterator rend()    const BOOST_NOEXCEPT {return reverse_iterator(begin());};
  const_reverse_iterator crbegin() const BOOST_NOEXCEPT {return reverse_iterator(end());};
  const_reverse_iterator crend()   const BOOST_NOEXCEPT {return reverse_iterator(begin());};
#else
  BOOST_CONSTEXPR const_reverse_iterator rbegin()  const BOOST_NOEXCEPT {return reverse_iterator(end());};
  BOOST_CONSTEXPR const_reverse_iterator rend()    const BOOST_NOEXCEPT {return reverse_iterator(begin());};
  BOOST_CONSTEXPR const_reverse_iterator crbegin() const BOOST_NOEXCEPT {return reverse_iterator(end());};
  BOOST_CONSTEXPR const_reverse_iterator crend()   const BOOST_NOEXCEPT {return reverse_iterator(begin());};
#endif

  BOOST_CONSTEXPR size_type size() const BOOST_NOEXCEPT {return length(); }
  BOOST_CONSTEXPR size_type length() const BOOST_NOEXCEPT {return length_impl_(); }
  BOOST_CONSTEXPR size_type max_size() const BOOST_NOEXCEPT {return static_cast<std::size_t>(-1); }
  BOOST_ATTRIBUTE_NODISCARD BOOST_CONSTEXPR bool empty() const BOOST_NOEXCEPT {return *view_ ==  '\0'; }

  BOOST_CONSTEXPR const_reference operator[](size_type pos) const  {return view_[pos] ;}
  BOOST_CXX14_CONSTEXPR const_reference at(size_type pos) const
  {
    if (pos >= size())
      throw_exception(std::out_of_range("cstring-view out of range"));
    return view_[pos];
  }
  BOOST_CONSTEXPR const_reference front() const  {return *view_;}
  BOOST_CONSTEXPR const_reference back()  const  {return view_[length() - 1];}
  BOOST_CONSTEXPR const_pointer data()    const BOOST_NOEXCEPT  {return view_;}
  BOOST_CXX14_CONSTEXPR void remove_prefix(size_type n)  {view_ = view_ + n;}
  void swap(cstring_ref& s) BOOST_NOEXCEPT {std::swap(view_, s.view_);}

  size_type copy(value_type* s, size_type n, size_type pos = 0) const
  {
    return traits_type::copy(s, view_ + pos, n) - view_;
  }
  BOOST_CONSTEXPR cstring_ref substr(size_type pos = 0) const
  {
    return cstring_ref(view_ + pos);
  }

  BOOST_CXX14_CONSTEXPR string_view_type substr(size_type pos, size_type length) const
  {
    return string_view_type(view_).substr(pos, length);
  }

  BOOST_CXX14_CONSTEXPR int compare(cstring_ref x) const BOOST_NOEXCEPT
  {
    auto idx = 0u;
    for (; view_[idx] != null_char_()[0] && x[idx] != null_char_()[0]; idx++)
      if (!traits_type::eq(view_[idx], x[idx]))
        return traits_type::lt(view_[idx], x[idx]) ? -1 : 1;

    return traits_type::to_int_type(view_[idx]) -
           traits_type::to_int_type(x[idx]); // will compare to null char of either.
  }

  BOOST_CXX14_CONSTEXPR bool starts_with(string_view_type x) const BOOST_NOEXCEPT
  {
    if (x.empty())
      return true;

    auto idx = 0u;
    for (; view_[idx] != null_char_()[0] && idx < x.size(); idx++)
      if (!traits_type::eq(view_[idx], x[idx]))
        return false;

    return idx == x.size() || view_[idx] != null_char_()[0];
  }
  BOOST_CONSTEXPR bool starts_with(value_type x)       const BOOST_NOEXCEPT
  {
    return traits_type::eq(view_[0], x);
  }

  BOOST_CXX14_CONSTEXPR size_type find( char ch, size_type pos = 0 ) const BOOST_NOEXCEPT
  {
    for (auto p = view_ + pos; *p != *null_char_(); p++)
      if (traits_type::eq(*p, ch))
        return p - view_;
    return npos;
  }


  friend BOOST_CXX14_CONSTEXPR bool operator==(cstring_ref x, cstring_ref y) BOOST_NOEXCEPT
  {
    std::size_t idx = 0u;
    for (idx = 0u; x[idx] != null_char_()[0] && y[idx] != null_char_()[0]; idx++)
      if (!traits_type::eq(x[idx], y[idx]))
        return false;
    return x[idx] == y[idx];
  }
  friend BOOST_CXX14_CONSTEXPR bool operator!=(cstring_ref x, cstring_ref y) BOOST_NOEXCEPT
  {
    std::size_t idx = 0u;
    for (idx = 0u; x[idx] != null_char_()[0] &&
                   y[idx] != null_char_()[0]; idx++)
      if (!traits_type::eq(x[idx], y[idx]))
        return true;
    return x[idx] != y[idx];
  }
  friend BOOST_CXX14_CONSTEXPR bool operator< (cstring_ref x, cstring_ref y) BOOST_NOEXCEPT {return x.compare(y) <  0;}
  friend BOOST_CXX14_CONSTEXPR bool operator> (cstring_ref x, cstring_ref y) BOOST_NOEXCEPT {return x.compare(y) >  0;}
  friend BOOST_CXX14_CONSTEXPR bool operator<=(cstring_ref x, cstring_ref y) BOOST_NOEXCEPT {return x.compare(y) <= 0;}
  friend BOOST_CXX14_CONSTEXPR bool operator>=(cstring_ref x, cstring_ref y) BOOST_NOEXCEPT {return x.compare(y) >= 0;}

  // modifiers
  void clear() BOOST_NOEXCEPT { view_ =  null_char_(); }          // Boost extension

  std::basic_string<value_type, traits_type> to_string() const
  {
    return std::basic_string<char, traits_type>(begin(), end());
  }

  template<typename Allocator>
  std::basic_string<value_type, traits_type, Allocator> to_string(const Allocator& a) const
  {
    return std::basic_string<value_type, traits_type, Allocator>(begin(), end(), a);
  }

  template<class A> operator std::basic_string<char, std::char_traits<char>, A>() const
  {
    return std::basic_string<char, std::char_traits<char>, A>( view_ );
  }

 private:
  BOOST_CONSTEXPR static const_pointer   null_char_()         {return "\0";}
  constexpr std::size_t length_impl_(std::size_t n = 0) const BOOST_NOEXCEPT
  {
    return view_[n] == null_char_()[0] ? n : length_impl_(n+1);
  }
  const_pointer view_;
};

template<typename Char>
inline std::basic_ostream<Char>& operator<<( std::basic_ostream<Char> & os, cstring_ref str )
{
  return os << core::string_view(str);
}


}
}


#endif //BOOST_SQLITE_CSTRING_REF_HPP
