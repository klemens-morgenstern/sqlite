//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_BACKUP_HPP
#define BOOST_SQLITE_BACKUP_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/connection_ref.hpp>
#include <boost/sqlite/cstring_ref.hpp>
#include <boost/sqlite/error.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE

struct connection ;

///@{
/**
 @brief Backup a database
 @ingroup reference

 This function will create a backup of an existing database.
 This can be useful to write an in memory database to disk et vice versa.

 @param source The source database to backup
 @param target The target of the backup
 @param source_name The source database to read the backup from. Default is 'main'.
 @param target_name The target database to write the backup to.  Default is 'main'.

 @par Error Handling

 @throws system_error from overload without `ec` & `ei`

 or you need to pass

 @param ec The system::error_code to capture any possibly errors
 @param ei Additional error_info when error occurs.

 @par Example

 @code{.cpp}

 sqlite::connection conn{":memory:"};
 {
    sqlite::connection read{"./read_only_db.db", SQLITE_READONLY};
    backup(read, target);
 }

 @endcode
 */
BOOST_SQLITE_DECL
void
backup(connection_ref source,
       connection_ref target,
       cstring_ref source_name,
       cstring_ref target_name,
       system::error_code & ec,
       error_info & ei);

BOOST_SQLITE_DECL
void
backup(connection_ref source,
       connection_ref target,
       cstring_ref source_name = "main",
       cstring_ref target_name = "main");

///@}

BOOST_SQLITE_END_NAMESPACE


#endif //BOOST_SQLITE_BACKUP_HPP
