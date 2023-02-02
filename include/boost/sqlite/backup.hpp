//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_BACKUP_HPP
#define BOOST_SQLITE_BACKUP_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/connection.hpp>

namespace boost
{
namespace sqlite
{


inline void
  backup(connection & source,
         connection & target,
         const std::string & source_name,
         const std::string & sink_name,
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
        sqlite3_backup_init(target.native_handle(), sink_name.c_str(),
                            source.native_handle(), source_name.c_str())};
  if (bu != nullptr)
  {
    BOOST_SQLITE_ASSIGN_EC(ec, sqlite3_errcode(target.native_handle()));
    ei.set_message(sqlite3_errmsg(target.native_handle()));
    return ;
  }

  const auto res = sqlite3_backup_step(bu.get(), -1);
  if (SQLITE_DONE != res)
    BOOST_SQLITE_ASSIGN_EC(ec, res);
}


inline void
backup(connection & source,
       connection & target,
       const std::string & source_name = "main",
       const std::string & sink_name = "main")
{
  system::error_code ec;
  error_info ei;
  backup(source, target, source_name, sink_name);
  if (ec)
    throw_exception(system::system_error(ec, ei.message()));
}

}
}

#endif //BOOST_SQLITE_BACKUP_HPP
