// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_VALUE_HPP
#define BOOST_SQLITE_VALUE_HPP

#include <sqlite3.h>
#include <boost/sqlite/value.hpp>

namespace boost {
namespace sqlite {

enum class value_type
{
    integer = SQLITE_INTEGER,
    floating = SQLITE_FLOAT,
    text = SQLITE_TEXT,
    blob = SQLITE_BLOB,
    null = SQLITE_NULL,
};



struct value
{
    value_type type() const
    {
        return static_cast<value_type>(sqlite3_value_type(value_));
    }

    bool is_null() const
    {
        return type() == value_type::null;
    }

    explicit operator bool () const
    {
        return type() != value_type::null;
    }

    int get_int() const
    {
        return sqlite3_value_int(value_);
    }
    sqlite3_int64 get_int64() const
    {
        return sqlite3_value_int64(value_);
    }

    double get_double() const
    {
        return sqlite3_value_double(value_);
    }

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

  private:
    sqlite3_value * value_ = nullptr;
};

static_assert(sizeof(value) == sizeof(sqlite3_value*));

}
}

#endif //BOOST_SQLITE_VALUE_HPP
