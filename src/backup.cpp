//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


#include <boost/sqlite/backup.hpp>
#include <boost/sqlite/connection.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE


void
backup(connection & source,
       connection & target,
       cstring_ref source_name,
       cstring_ref target_name,
       system::error_code & ec,
       error_info & ei)
{
  struct del
  {
    void operator()(sqlite3_backup * bp)
    {
      sqlite3_backup_finish(bp);
    }
  };

  std::unique_ptr<sqlite3_backup, del> bu{
      sqlite3_backup_init(target.handle(), target_name.c_str(),
                          source.handle(), source_name.c_str())};
  if (bu == nullptr)
  {
    BOOST_SQLITE_ASSIGN_EC(ec, sqlite3_errcode(target.handle()));
    ei.set_message(sqlite3_errmsg(target.handle()));
    return ;
  }

  const auto res = sqlite3_backup_step(bu.get(), -1);
  if (SQLITE_DONE != res)
    BOOST_SQLITE_ASSIGN_EC(ec, res);
}


void
backup(connection & source,
       connection & target,
       cstring_ref source_name,
       cstring_ref target_name)
{
  system::error_code ec;
  error_info ei;
  backup(source, target, source_name, target_name, ec, ei);
  if (ec)
    throw_exception(system::system_error(ec, ei.message()));
}

BOOST_SQLITE_END_NAMESPACE

