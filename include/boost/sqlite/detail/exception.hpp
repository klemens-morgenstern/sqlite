// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_DETAIL_EXCEPTION_HPP
#define BOOST_SQLITE_DETAIL_EXCEPTION_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/error.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE
namespace detail
{

BOOST_NORETURN void throw_error_code(const boost::system::error_code & ec,
                                     const boost::source_location & loc = BOOST_CURRENT_LOCATION);
BOOST_NORETURN void throw_error_code(const boost::system::error_code & ec,
                                     const error_info & ei,
                                     const boost::source_location & loc = BOOST_CURRENT_LOCATION);
BOOST_NORETURN void throw_out_of_range(const char * msg,
                                       const boost::source_location & loc);

BOOST_NORETURN void throw_invalid_argument(const char * msg,
                                           const boost::source_location & loc);


}
BOOST_SQLITE_END_NAMESPACE


#endif //BOOST_SQLITE_DETAIL_EXCEPTION_HPP
