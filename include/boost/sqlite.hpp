// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_HPP
#define BOOST_SQLITE_HPP

/** @defgroup reference Reference
 *
 *  This page contains the documentation of the sqlite high-level API.
 */

#include <boost/sqlite/backup.hpp>
#include <boost/sqlite/blob.hpp>
#include <boost/sqlite/collation.hpp>
#include <boost/sqlite/connection.hpp>
#include <boost/sqlite/cstring_ref.hpp>
#include <boost/sqlite/error.hpp>
#include <boost/sqlite/field.hpp>
#include <boost/sqlite/function.hpp>
#include <boost/sqlite/meta_data.hpp>
#include <boost/sqlite/memory.hpp>
#include <boost/sqlite/hooks.hpp>
#include <boost/sqlite/json.hpp>
#include <boost/sqlite/resultset.hpp>
#include <boost/sqlite/row.hpp>
#include <boost/sqlite/statement.hpp>
#include <boost/sqlite/static_resultset.hpp>
#include <boost/sqlite/string.hpp>
#include <boost/sqlite/value.hpp>
#include <boost/sqlite/vtable.hpp>

#if defined(BOOST_SQLITE_COMPILE_EXTENSION)
#include <boost/sqlite/extension.hpp>
#endif

#endif //BOOST_SQLITE_HPP
