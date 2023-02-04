// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_VALUE_HPP
#define BOOST_SQLITE_VALUE_HPP

#include <sqlite3.h>
#include <boost/sqlite/blob.hpp>
#include <boost/core/detail/string_view.hpp>


namespace boost {
namespace sqlite {

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

/** @brief A holder for a sqlite values used for internal APIs
    @ingroup reference

 */
struct value
{
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
    /// Get the value as regular `int`.
    int get_int() const
    {
        return sqlite3_value_int(value_);
    }
    /// Get the value as an `int64`.
    sqlite3_int64 get_int64() const
    {
        return sqlite3_value_int64(value_);
    }
    /// Get the value as an `double`.
    double get_double() const
    {
        return sqlite3_value_double(value_);
    }
    /// Get the value as text, i.e. a string_view. Note that this value may be invalidated`.
    core::string_view get_text() const
    {
        const auto ptr = sqlite3_value_text(value_);
        if (ptr == nullptr)
        {
            if (is_null())
                return "";
            else
                throw_exception(std::bad_alloc());
        }
        const auto sz = sqlite3_value_bytes(value_);
        return core::string_view(reinterpret_cast<const char*>(ptr), sz);
    }
    /// Get the value as blob, i.e. raw memory. Note that this value may be invalidated`.
    blob_view get_blob() const
    {
        const auto ptr = sqlite3_value_blob(value_);
        if (ptr == nullptr)
        {
            if (is_null())
                return {nullptr, 0u};
            else
                throw_exception(std::bad_alloc());
        }
        const auto sz = sqlite3_value_bytes(value_);
        return blob_view(ptr, sz);
    }
    /// Construct value from a native_handle.
    explicit value(sqlite3_value * value_) noexcept : value_(value_) {}

    /// The native handle of the value.
    using native_handle_type = sqlite3_value *;
    /// Get the native_handle.
    native_handle_type native_handle() const {return value_;}
    native_handle_type & native_handle() {return value_;}

    /// Get a value that was passed through the pointer interface.
    /// A value can be set as a pointer by binding/returning a unique_ptr.
    template<typename T>
    T * get_pointer() {return static_cast<T*>(sqlite3_value_pointer(value_, typeid(T).name()));}


    sqlite3_value ** operator&() {return &value_;}
  private:
    sqlite3_value * value_ = nullptr;
};

static_assert(sizeof(value) == sizeof(sqlite3_value*));

}
}

#endif //BOOST_SQLITE_VALUE_HPP
