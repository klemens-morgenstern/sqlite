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

BOOST_SQLITE_BEGIN_NAMESPACE

blob_handle open_blob(connection & conn,
                      cstring_ref db,
                      cstring_ref table,
                      cstring_ref column,
                      sqlite3_int64 row,
                      bool read_only,
                      system::error_code & ec,
                      error_info & ei)
{
  sqlite3_blob * bb = nullptr;
  blob_handle bh;

  int res = sqlite3_blob_open(conn.handle(), db.c_str(), table.c_str(), column.c_str(),
                              row, read_only ? 0 : 1, &bb);
  if (res != 0)
  {
      BOOST_SQLITE_ASSIGN_EC(ec, sqlite3_errcode(conn.handle()))
      ei.set_message(sqlite3_errmsg(conn.handle()));
  }
  else
      bh = blob_handle(bb);

  return bh;
}

blob_handle open_blob(connection & conn,
                      cstring_ref db,
                      cstring_ref table,
                      cstring_ref column,
                      sqlite3_int64 row,
                      bool read_only)
{
  boost::system::error_code ec;
  error_info ei;
  auto b = open_blob(conn, db, table, column, row, read_only, ec, ei);
  if (ec)
    boost::throw_exception(system::system_error(ec, ei.message()), BOOST_CURRENT_LOCATION);
  return b;
}

void blob_handle::reopen(sqlite3_int64 row_id, system::error_code & ec)
{
  int res = sqlite3_blob_reopen(blob_.get(), row_id);
  BOOST_SQLITE_ASSIGN_EC(ec, res);
}
void blob_handle::reopen(sqlite3_int64 row_id)
{
  boost::system::error_code ec;
  reopen(row_id, ec);
  if (ec)
    boost::throw_exception(system::system_error(ec), BOOST_CURRENT_LOCATION);
}

void blob_handle::read_at(void *data, int len, int offset, system::error_code &ec)
{
  int res = sqlite3_blob_read(blob_.get(), data, len, offset);
  BOOST_SQLITE_ASSIGN_EC(ec, res);
}
void blob_handle::read_at(void *data, int len, int offset)
{
  boost::system::error_code ec;
  read_at(data, len, offset, ec);
  if (ec)
    boost::throw_exception(system::system_error(ec), BOOST_CURRENT_LOCATION);
}

void blob_handle::write_at(const void *data, int len, int offset, system::error_code &ec)
{
  int res = sqlite3_blob_write(blob_.get(), data, len, offset);
  BOOST_SQLITE_ASSIGN_EC(ec, res);
}
void blob_handle::write_at(const void *data, int len, int offset)
{
  boost::system::error_code ec;
  write_at(data, len, offset, ec);
  if (ec)
    boost::throw_exception(system::system_error(ec), BOOST_CURRENT_LOCATION);
}

BOOST_SQLITE_END_NAMESPACE

#endif //BOOST_SQLITE_IMPL_BLOB_IPP
