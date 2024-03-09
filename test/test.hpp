//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_TEST_HPP
#define BOOST_SQLITE_TEST_HPP

#include <boost/test/unit_test.hpp>

struct query_helper
{
  const char * query;
  query_helper(const char * query) : query(query) {}

  const char * operator--() const {return query;}
};


#endif //BOOST_SQLITE_TEST_HPP
