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
class error_info
{
    unique_ptr<char> msg_;

  public:
    /// Default constructor.
    error_info() = default;

    /// Initialization constructor.
    error_info(cstring_ref msg) noexcept : msg_(new (memory_tag{}) char[msg.size() + 1u])
    {
      std::strcpy(msg_.get(), msg.c_str());
    }

    /// set the message by copy
    void set_message(cstring_ref msg)
    {
      reserve(msg.size() + 1u);
      std::strcpy(msg_.get(), msg.c_str());
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
};


BOOST_SQLITE_END_NAMESPACE

#if defined(BOOST_SQLITE_HEADER_ONLY)
#include <boost/sqlite/impl/error.ipp>
#endif

#endif // BOOST_SQLITE_ERROR_HPP
