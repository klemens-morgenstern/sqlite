// Copyright (c) 2021 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_SRC_IPP
#define BOOST_SQLITE_SRC_IPP

#include <boost/sqlite/detail/config.hpp>

#if defined(BOOST_SQLITE_HEADER_ONLY)
#error "You can't include this in header only mode"
#endif

#ifndef BOOST_SQLITE_SOURCE
#define BOOST_SQLITE_SOURCE
#endif

#include <boost/sqlite/impl/backup.ipp>
#include <boost/sqlite/impl/connection.ipp>
#include <boost/sqlite/impl/error.ipp>
#include <boost/sqlite/impl/resultset.ipp>
#include <boost/sqlite/impl/json.ipp>

#endif //BOOST_SQLITE_SRC_IPP
