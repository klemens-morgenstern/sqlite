//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_META_DATA_HPP
#define BOOST_SQLITE_META_DATA_HPP

#include <boost/sqlite/connection.hpp>

namespace boost
{
namespace sqlite
{

struct connection ;

/// The metadata of a column
struct column_meta_data
{
    /// Data type fo the column
    string_view data_type;
    /// Name of default collation sequence
    string_view collation;
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
column_meta_data table_column_meta_data(connection  & conn,
                                        const char * db_name, const char * table_name, const char * column_name,
                                        error_code & ec, error_info &ei);
BOOST_SQLITE_DECL
column_meta_data table_column_meta_data(connection  & conn,
                                        const char * table_name, const char * column_name,
                                        error_code & ec, error_info &ei);

BOOST_SQLITE_DECL
column_meta_data table_column_meta_data(connection  & conn,
                                        const char * db_name, const char * table_name, const char * column_name);
BOOST_SQLITE_DECL
column_meta_data table_column_meta_data(connection  & conn,
                                        const char * table_name, const char * column_name);
///@}

///

}
}

#endif //BOOST_SQLITE_META_DATA_HPP
