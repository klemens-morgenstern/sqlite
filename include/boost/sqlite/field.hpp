// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_FIELD_HPP
#define BOOST_SQLITE_FIELD_HPP

#include <boost/variant2/variant.hpp>
#include <boost/utility/string_view.hpp>
#include <boost/sqlite/blob.hpp>
#include <boost/sqlite/value.hpp>

namespace boost {
namespace sqlite {

struct field
{
    value_type type() const
    {
        return static_cast<value_type>( sqlite3_column_type(stm_, col_));
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
        return  sqlite3_column_int(stm_, col_);
    }
    sqlite3_int64 get_int64() const
    {
        return  sqlite3_column_int64(stm_, col_);
    }

    double get_double() const
    {
        return  sqlite3_column_double(stm_, col_);
    }

    core::string_view get_text() const
    {
        const auto ptr =  sqlite3_column_text(stm_, col_);
        if (ptr == nullptr)
        {
            if (sqlite3_errcode(sqlite3_db_handle(stm_)) != SQLITE_NOMEM)
                return "";
            else
                throw_exception(std::bad_alloc());
        }
        const auto sz =  sqlite3_column_bytes(stm_, col_);
        return core::string_view(reinterpret_cast<const char*>(ptr), sz);
    }

    blob_view get_blob() const
    {
        const auto ptr =  sqlite3_column_blob(stm_, col_);
        if (ptr == nullptr)
        {
            if (sqlite3_errcode(sqlite3_db_handle(stm_)) != SQLITE_NOMEM)
                return {nullptr, 0u};
            else
                throw_exception(std::bad_alloc());
        }
        const auto sz =  sqlite3_column_bytes(stm_, col_);
        return blob_view(ptr, sz);
    }

    value get_value() const
    {
      return value(sqlite3_column_value(stm_, col_));
    }

    core::string_view column_name() const
    {
      return sqlite3_column_name(stm_, col_);
    }

    core::string_view table_name() const
    {
      return sqlite3_column_table_name(stm_, col_);
    }

    core::string_view column_origin_name() const
    {
      return sqlite3_column_origin_name(stm_, col_);
    }

  private:
    friend struct row;
    sqlite3_stmt * stm_;
    int col_ = -1;
};

}
}

#endif //BOOST_SQLITE_FIELD_HPP
