//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_IMPL_BLOB_IPP
#define BOOST_SQLITE_IMPL_BLOB_IPP

#include <boost/sqlite/blob.hpp>
#include <boost/sqlite/connection.hpp>

namespace boost
{
namespace sqlite
{

blob_handle open_blob(connection & conn,
                      const char *db,
                      const char * table,
                      const char * column,
                      sqlite3_int64 row,
                      bool read_only,
                      error_code & ec,
                      error_info & ei)
{
  sqlite3_blob * bb = nullptr;
  blob_handle bh;

  int res = sqlite3_blob_open(conn.handle(), db, table, column, row, read_only ? 0 : 1, &bb);
  if (res != 0)
  {
      BOOST_SQLITE_ASSIGN_EC(ec, sqlite3_errcode(conn.handle()))
      ei.set_message(sqlite3_errmsg(conn.handle()));
  }
  else
      bh.blob_.reset(bb);

  return bh;
}

blob_handle open_blob(connection & conn,
                      const char *db,
                      const char * table,
                      const char * column,
                      sqlite3_int64 row,
                      bool read_only)
{
  boost::system::error_code ec;
  error_info ei;
  auto b = open_blob(conn, db, table, column, row, read_only, ec, ei);
  if (ec)
    boost::throw_exception(system_error(ec, ei.message()));
  return b;
}

}
}

#endif //BOOST_SQLITE_IMPL_BLOB_IPP
