// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_FIELD_HPP
#define BOOST_SQLITE_FIELD_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/blob.hpp>
#include <boost/sqlite/cstring_ref.hpp>
#include <boost/sqlite/value.hpp>

#include <boost/core/detail/string_view.hpp>


BOOST_SQLITE_BEGIN_NAMESPACE

/** @brief A holder for a sqlite field, i.e. something returned from a query.
    @ingroup reference
 */
struct field
{
    typedef sqlite_int64 int64;

    /// The type of the value
    value_type type() const
    {
        return static_cast<value_type>( sqlite3_column_type(stm_, col_));
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
    /// Returns the value as an `int64`.
    int64 get_int() const
    {
        return  sqlite3_column_int64(stm_, col_);
    }
    /// Returns the value as an `double`.
    double get_double() const
    {
        return  sqlite3_column_double(stm_, col_);
    }
    /// Returns the value as text, i.e. a string_view. Note that this value may be invalidated`.
    BOOST_SQLITE_DECL
    cstring_ref get_text() const;
    /// Returns the value as blob, i.e. raw memory. Note that this value may be invalidated`.
    BOOST_SQLITE_DECL
    blob_view get_blob() const;
    /// Returns the field as a value.
    value get_value() const
    {
      return value(sqlite3_column_value(stm_, col_));
    }
    /// Returns the name of the column.
    cstring_ref column_name() const
    {
      return sqlite3_column_name(stm_, col_);
    }
    /// Returns the name of the table.
    cstring_ref table_name() const
    {
      return sqlite3_column_table_name(stm_, col_);
    }
    /// Returns the name of the original data source.
    cstring_ref column_origin_name() const
    {
      return sqlite3_column_origin_name(stm_, col_);
    }

  private:
    friend struct row;
    sqlite3_stmt * stm_;
    int col_ = -1;
};

BOOST_SQLITE_END_NAMESPACE

#endif //BOOST_SQLITE_FIELD_HPP
