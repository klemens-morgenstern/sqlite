//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_RESULT_CATCH_HPP
#define BOOST_SQLITE_RESULT_CATCH_HPP

#include <stdexcept>

#define BOOST_SQLITE_CATCH_RESULT(ctx)                                             \
catch (boost::system::system_error & se)                                           \
{                                                                                  \
  auto code = SQLITE_ERROR;                                                        \
  if (se.code().category() == boost::sqlite::sqlite_category())                    \
    code = se.code().value();                                                      \
  sqlite3_result_error(ctx, se.what(), code);                                      \
}                                                                                  \
catch(std::bad_alloc    &) { sqlite3_result_error_nomem(ctx); }                    \
catch(std::length_error &) { sqlite3_result_error_toobig(ctx); }                   \
catch(std::exception & ex) { sqlite3_result_error(ctx, ex.what(), SQLITE_ERROR); }



#endif //BOOST_SQLITE_RESULT_CATCH_HPP
