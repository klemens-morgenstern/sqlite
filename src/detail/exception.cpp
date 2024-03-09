// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/sqlite/detail/exception.hpp>
#include <boost/system/system_error.hpp>

#include <stdexcept>

BOOST_SQLITE_BEGIN_NAMESPACE
namespace detail
{

void throw_error_code(const boost::system::error_code & ec,
                      const boost::source_location & loc)
{
  boost::throw_exception(system::system_error(ec),
                         ec.has_location() ? ec.location() : loc);
}

void throw_error_code(const boost::system::error_code & ec,
                      const error_info & ei,
                      const boost::source_location & loc)
{
  boost::throw_exception(system::system_error(ec, ei.message()),
                         ec.has_location() ? ec.location() : loc);
}

void throw_out_of_range(const char * msg,
                        const boost::source_location & loc)
{
  boost::throw_exception(std::out_of_range(msg), loc);
}

void throw_invalid_argument(const char * msg,
                             const boost::source_location & loc)
{
  {
    boost::throw_exception(std::invalid_argument(msg), loc);
  }
}


}
BOOST_SQLITE_END_NAMESPACE
