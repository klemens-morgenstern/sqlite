//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_ERROR_HPP
#define BOOST_SQLITE_ERROR_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/cstring_ref.hpp>
#include <boost/sqlite/memory.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/error_category.hpp>
#include <boost/system/result.hpp>


BOOST_SQLITE_BEGIN_NAMESPACE

BOOST_SQLITE_DECL
system::error_category & sqlite_category();


/**
 * \brief Additional information about error conditions stored in an sqlite-allocate string
 * \ingroup reference
 * \details Contains an error message describing what happened. Not all error
 * conditions are able to generate this extended information - those that
 * can't have an empty error message.
 */
struct error_info
{
    /// Default constructor.
    error_info() = default;

    /// Initialization constructor.
    error_info(core::string_view msg) noexcept : msg_(new (memory_tag{}) char[msg.size() + 1u])
    {
      std::memcpy(msg_.get(), msg.data(), msg.size() + 1);
    }

    /// set the message by copy
    void set_message(core::string_view msg)
    {
      reserve(msg.size() + 1u);
      std::memcpy(msg_.get(), msg.data(), msg.size() + 1);
    }
    /// transfer ownership into the message
    void reset(char * c = nullptr)
    {
      msg_.reset(c);
    }

    /// use sqlite_mprintf to generate a message
    cstring_ref format(cstring_ref fmt, ...)
    {
      va_list args;
      va_start(args, fmt);
      msg_.reset(sqlite3_vmprintf(fmt.c_str(), args));
      va_end(args);
      return msg_.get();
    }

    /// use sqlite_snprintf to generate a message
    cstring_ref snformat(cstring_ref fmt, ...)
    {
      if (capacity() == 0)
        return "";
      va_list args;
      va_start(args, fmt);
      sqlite3_vsnprintf(capacity(), msg_.get(), fmt.c_str(), args);
      va_end(args);
      return msg_.get();
    }
    /// reserve data in the buffer i.e. allocate
    void reserve(std::size_t sz)
    {
      if (msg_)
      {
        if (sqlite3_msize(msg_.get()) < sz)
          msg_.reset(static_cast<char*>(sqlite3_realloc64(msg_.release(), sz)));
      }
      else
        msg_.reset(static_cast<char*>(sqlite3_malloc64(sz)));
    }

    /// Get the allocated memory
    std::size_t capacity() const {return msg_ ? sqlite3_msize(msg_.get()) : 0u;}

    /// Gets the error message.
    cstring_ref message() const noexcept { return msg_ ? msg_.get() : ""; }

    char * release()
    {
      return msg_.release();
    }
    /// Restores the object to its initial state.
    void clear() noexcept { if (msg_) *msg_ = '\0'; }


    operator bool() const {return msg_.operator bool();}
  private:
    unique_ptr<char> msg_;
};


/**
 * \brief An error containing both a code & optional message.
 * \ingroup reference
 * \details Contains an error .
 */
struct error
{
  /// The code of the error.
  int code;
  /// The additional information of the error
  error_info info;

  error(int code, error_info info) : code(code), info(std::move(info)) {}
  explicit error(int code)                  : code(code)                        {}
  error(int code, core::string_view info)
          : code(code),
            info(info) {}

  error(system::error_code code, error_info info)
          : code(code.category() == sqlite_category() ? code.value() : SQLITE_FAIL),
            info(std::move(info)) {}

  error(system::error_code code) : code(code.category() == sqlite_category() ? code.value() : SQLITE_FAIL)
  {
    if (code.category() == sqlite_category())
      info = error_info{code.what()};
  }
  error() = default;
  error(error && ) noexcept = default;
};

BOOST_NORETURN BOOST_SQLITE_DECL void throw_exception_from_error( error const & e, boost::source_location const & loc );

template<typename T = void>
using result = system::result<T, error>;

template<typename T>
struct is_result_type : std::false_type {};

template<typename T>
struct is_result_type<result<T>> : std::true_type {};


BOOST_SQLITE_END_NAMESPACE


#endif // BOOST_SQLITE_ERROR_HPP
