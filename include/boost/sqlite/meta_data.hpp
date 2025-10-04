//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_META_DATA_HPP
#define BOOST_SQLITE_META_DATA_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/connection_ref.hpp>
#include <boost/sqlite/cstring_ref.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE


struct connection ;

/// The metadata of a column
struct column_meta_data
{
    /// Data type fo the column
    cstring_ref data_type;
    /// Name of default collation sequence
    cstring_ref collation;
    /// true if column has a NOT NULL constraint
    bool not_null;
    /// true if column is part of the PRIMARY KEY
    bool primary_key;
    /// true if column is AUTOINCREMENT
    bool auto_increment;
};

///@{
/// get the meta-data of one colum
BOOST_SQLITE_DECL
column_meta_data table_column_meta_data(connection_ref conn,
                                        cstring_ref db_name, cstring_ref table_name, cstring_ref column_name,
                                        system::error_code & ec, error_info &ei);
BOOST_SQLITE_DECL
column_meta_data table_column_meta_data(connection_ref conn,
                                        cstring_ref table_name, cstring_ref column_name,
                                        system::error_code & ec, error_info &ei);

BOOST_SQLITE_DECL
column_meta_data table_column_meta_data(connection_ref conn,
                                        cstring_ref db_name, cstring_ref table_name, cstring_ref column_name);
BOOST_SQLITE_DECL
column_meta_data table_column_meta_data(connection_ref conn,
                                        cstring_ref table_name, cstring_ref column_name);
///@}

///
BOOST_SQLITE_END_NAMESPACE


#endif //BOOST_SQLITE_META_DATA_HPP
