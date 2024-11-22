// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_VALUE_HPP
#define BOOST_SQLITE_VALUE_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/blob.hpp>
#include <boost/sqlite/cstring_ref.hpp>


BOOST_SQLITE_BEGIN_NAMESPACE

/** @brief The type of a value
  @ingroup reference

  [related sqlite documentation](https://www.sqlite.org/datatype3.html)
*/
enum class value_type
{
    /// An integral value
    integer = SQLITE_INTEGER,
    /// A floating piont value
    floating = SQLITE_FLOAT,
    /// A textual value
    text = SQLITE_TEXT,
    /// A binary value
    blob = SQLITE_BLOB,
    /// No value
    null = SQLITE_NULL,
};

inline
const char * value_type_name(value_type vt)
{
    switch (vt)
    {
        case value_type::text:      return "text";
        case value_type::blob:      return "blob";
        case value_type::floating:  return "floating";
        case value_type::integer:   return "integer";
        case value_type::null:      return "null";
        default: return "unknown";
    }
}


/** @brief A holder for a sqlite values used for internal APIs
    @ingroup reference

 */
struct value
{
    // The value for integers in the database
    typedef sqlite3_int64 int64 ;

    /// The type of the value
    value_type type() const
    {
        return static_cast<value_type>(sqlite3_value_type(value_));
    }
    /// The subtype of the value, see
    int subtype() const
    {
        return sqlite3_value_subtype(value_);
    }
    /// Is the held value null
    bool is_null() const
    {
        return type() == value_type::null;
    }
    /// Is the held value is not null
    explicit operator bool () const
    {
        return type() != value_type::null;
    }
    /// Returns the value as an `integer`.
    int64 get_int() const
    {
        return sqlite3_value_int64(value_);
    }
    /// Returns the value as an `double`.
    double get_double() const
    {
        return sqlite3_value_double(value_);
    }
    /// Returns the value as text, i.e. a string_view. Note that this value may be invalidated`.
    BOOST_SQLITE_DECL
    cstring_ref get_text() const;
    /// Returns the value as blob, i.e. raw memory. Note that this value may be invalidated`.
    BOOST_SQLITE_DECL
    blob_view get_blob() const;

    /// Best numeric datatype of the value
    value_type numeric_type()  const{return static_cast<value_type>(sqlite3_value_numeric_type(value_));}

#if SQLITE_VERSION_NUMBER >= 3032000
    /// True if the column is unchanged in an UPDATE against a virtual table.
    bool nochange() const {return 0 != sqlite3_value_nochange(value_);}
#endif

#if SQLITE_VERSION_NUMBER >= 3031000
    /// True if value originated from a bound parameter
    bool from_bind() const {return 0 != sqlite3_value_frombind(value_);}
#endif
    /// Construct value from a handle.
    explicit value(sqlite3_value * value_) noexcept : value_(value_) {}

    /// The handle of the value.
    using handle_type = sqlite3_value *;
    /// Returns the handle.
    handle_type handle() const {return value_;}
    handle_type & handle() {return value_;}

#if SQLITE_VERSION_NUMBER >= 3020000
    /// Get a value that was passed through the pointer interface.
    /// A value can be set as a pointer by binding/returning a unique_ptr.
    template<typename T>
    T * get_pointer() {return static_cast<T*>(sqlite3_value_pointer(value_, typeid(T).name()));}
#endif
  private:
    sqlite3_value * value_ = nullptr;
};

static_assert(sizeof(value) == sizeof(sqlite3_value*), "value must be same as sqlite3_value* pointer");

BOOST_SQLITE_END_NAMESPACE

#endif //BOOST_SQLITE_VALUE_HPP
