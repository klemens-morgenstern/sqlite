// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/sqlite/value.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE

cstring_ref value::get_text() const
{
  const auto ptr = sqlite3_value_text(value_);
  if (ptr == nullptr)
  {
    if (is_null())
      return "";
    else
      throw_exception(std::bad_alloc(), BOOST_CURRENT_LOCATION);
  }
  return reinterpret_cast<const char*>(ptr);
}

blob_view value::get_blob() const
{
  const auto ptr = sqlite3_value_blob(value_);
  if (ptr == nullptr)
  {
    if (is_null())
      return {nullptr, 0u};
    else
      throw_exception(std::bad_alloc(), BOOST_CURRENT_LOCATION);
  }
  const auto sz = sqlite3_value_bytes(value_);
  return blob_view(ptr, sz);
}


BOOST_SQLITE_END_NAMESPACE

