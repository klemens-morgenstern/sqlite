// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_RESULTSET_HPP
#define BOOST_SQLITE_RESULTSET_HPP

#include <memory>
#include <sqlite3.h>
#include <boost/sqlite/row.hpp>

namespace boost {
namespace sqlite {

struct resultset
{

    row current() const
    {
        row r;
        r.stm_ = impl_.get();
        return r;
    }

    bool done() const {return done_;}

    bool read_one(row& r,
                  error_code & ec,
                  error_info & ei) // could also return row* instead!
    {
        if (done_)
            return false;
        auto cc = sqlite3_step(impl_.get());
        if (cc == SQLITE_DONE)
        {
            done_ = true;
            return false;
        }
        else if (cc == SQLITE_ROW)
            r.stm_ = impl_.get();
        else
        {
            BOOST_SQLITE_ASSIGN_EC(ec, cc);
            ei.set_message(sqlite3_errmsg(sqlite3_db_handle(impl_.get())));
        }
        return !done_;
    }

    bool read_one(row & r)
    {
        system::error_code ec;
        error_info ei;
        auto tmp = read_one(r, ec, ei);
        if (ec)
            throw_exception(system::system_error(ec, ei.message()));
        return tmp;
    }
  private:
    friend struct connection;
    friend struct statement;
    struct deleter_
    {
        void operator()(sqlite3_stmt * sm)
        {
            while ( sqlite3_step(sm) == SQLITE_ROW);
            sqlite3_finalize(sm);
        }
    };
    std::unique_ptr<sqlite3_stmt, deleter_> impl_;
    bool done_ = false;
};

}
}

#endif //BOOST_SQLITE_RESULTSET_HPP
