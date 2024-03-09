// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/sqlite/row.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE

field row::at(std::size_t idx) const
{
  if (idx >= size())
    throw_exception(std::out_of_range("column out of range"), BOOST_CURRENT_LOCATION);
  else
  {
    field f;
    f.stm_ = stm_;
    f.col_ = static_cast<int>(idx);
    return f;
  }
}

BOOST_SQLITE_END_NAMESPACE

