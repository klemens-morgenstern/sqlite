//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_DETAIL_CATCH_HPP
#define BOOST_SQLITE_DETAIL_CATCH_HPP

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
catch(std::out_of_range &) { sqlite3_result_error_code(ctx, SQLITE_RANGE);}        \
catch(std::logic_error &le) { sqlite3_result_error(ctx, le.what(), SQLITE_MISUSE);}\
catch(std::exception & ex) { sqlite3_result_error(ctx, ex.what(), SQLITE_ERROR); } \
catch(...) {sqlite3_result_error_code(ctx, SQLITE_ERROR); }

#define BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(msg)                              \
catch (boost::system::system_error & se)                                           \
{                                                                                  \
  auto code = SQLITE_ERROR;                                                        \
  if (se.code().category() == boost::sqlite::sqlite_category())                    \
    code = se.code().value();                                                      \
  msg = sqlite3_mprintf("%s", se.what());                                          \
  return code;                                                                     \
}                                                                                  \
catch(std::bad_alloc    &) { return SQLITE_NOMEM; }                                \
catch(std::length_error &) { return SQLITE_TOOBIG; }                               \
catch(std::out_of_range &) { return SQLITE_RANGE;}                                 \
catch(std::logic_error &le)                                                        \
{                                                                                  \
    msg = sqlite3_mprintf("%s", le.what());                                        \
    return SQLITE_MISUSE;                                                          \
}                                                                                  \
catch(std::exception & ex)                                                         \
{                                                                                  \
    msg = sqlite3_mprintf("%s", ex.what());                                        \
    return SQLITE_ERROR;                                                           \
}                                                                                  \
catch(...) {return SQLITE_ERROR; }                                                 \

#define BOOST_SQLITE_CATCH_AND_RETURN()                                            \
catch (boost::system::system_error & se)                                           \
{                                                                                  \
  auto code = SQLITE_ERROR;                                                        \
  if (se.code().category() == boost::sqlite::sqlite_category())                    \
    code = se.code().value();                                                      \
  return code;                                                                     \
}                                                                                  \
catch(std::bad_alloc    &) { return SQLITE_NOMEM; }                                \
catch(std::length_error &) { return SQLITE_TOOBIG; }                               \
catch(std::out_of_range &) { return SQLITE_RANGE;}                                 \
catch(std::logic_error  &) { return SQLITE_MISUSE;}                                \
catch(...) { return SQLITE_ERROR; }



#endif //BOOST_SQLITE_DETAIL_CATCH_HPP
