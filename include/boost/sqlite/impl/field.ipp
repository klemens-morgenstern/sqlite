// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_IMPL_FIELD_IPP
#define BOOST_SQLITE_IMPL_FIELD_IPP

#include <boost/sqlite/field.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE

core::string_view field::get_text() const
{
  const auto ptr =  sqlite3_column_text(stm_, col_);
  if (ptr == nullptr)
  {
    if (sqlite3_errcode(sqlite3_db_handle(stm_)) != SQLITE_NOMEM)
      return "";
    else
      throw_exception(std::bad_alloc(), BOOST_CURRENT_LOCATION);
  }
  const auto sz =  sqlite3_column_bytes(stm_, col_);
  return core::string_view(reinterpret_cast<const char*>(ptr), sz);
}


blob_view field::get_blob() const
{
  const auto ptr =  sqlite3_column_blob(stm_, col_);
  if (ptr == nullptr)
  {
    if (sqlite3_errcode(sqlite3_db_handle(stm_)) != SQLITE_NOMEM)
      return {nullptr, 0u};
    else
      throw_exception(std::bad_alloc(), BOOST_CURRENT_LOCATION);
  }
  const auto sz =  sqlite3_column_bytes(stm_, col_);
  return blob_view(ptr, sz);
}

BOOST_SQLITE_END_NAMESPACE

#endif //BOOST_SQLITE_IMPL_FIELD_IPP
