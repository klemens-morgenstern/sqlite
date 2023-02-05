//
// Copyright (c) 2023 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#define BOOST_SQLITE_COMPILE_EXTENSION 1
#include <boost/sqlite/src.hpp>


BOOST_SQLITE_BEGIN_NAMESPACE

BOOST_SYMBOL_EXPORT const sqlite3_api_routines *sqlite3_api{nullptr};

BOOST_SQLITE_END_NAMESPACE
