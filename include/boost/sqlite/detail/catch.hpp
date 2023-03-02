//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_DETAIL_CATCH_HPP
#define BOOST_SQLITE_DETAIL_CATCH_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/error.hpp>

#include <stdexcept>
#include <boost/leaf/handle_errors.hpp>
#include <boost/leaf/result.hpp>
#include <boost/system/system_error.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE
namespace detail
{

template<typename Func>
void execute_context_function(sqlite3_context * ctx, Func && func) noexcept
{
  return leaf::try_handle_all(
      std::forward<Func>(func),
      [ctx](system::system_error & se)
      {
        if (se.code().category() == boost::sqlite::sqlite_category())
          sqlite3_result_error_code(ctx, se.code().value());
        else
          sqlite3_result_error_code(ctx, SQLITE_ERROR);
        sqlite3_result_error(ctx, se.what(), std::strlen(se.what()));
      },
      [ctx](std::bad_alloc    &) { sqlite3_result_error_nomem(ctx); },
      [ctx](std::length_error &) { sqlite3_result_error_toobig(ctx); },
      [ctx](std::out_of_range &) { sqlite3_result_error_code(ctx, SQLITE_RANGE);},
      [ctx](std::length_error &) { sqlite3_result_error_toobig(ctx); },
      [ctx](std::logic_error &le)
      {
        sqlite3_result_error_code(ctx, SQLITE_MISUSE);
        sqlite3_result_error(ctx, le.what(), std::strlen(le.what()));
      },
      [ctx](std::exception & ex)
      {
        sqlite3_result_error(ctx, ex.what(), std::strlen(ex.what()));
      },
      [ctx](int id, error_info &ei)
      {
        sqlite3_result_error_code(ctx, id);
        sqlite3_result_error(ctx, ei.message().data(), ei.message().size());
      },
      [ctx](int id)
      {
        sqlite3_result_error_code(ctx, id);
      },
      [ctx]
      {
        sqlite3_result_error_code(ctx, SQLITE_ERROR);
      }
      );
}


template<typename Func>
int execute_function_return(char * &msg, Func && func) noexcept
{
  return leaf::try_handle_all(
      std::forward<Func>(func),
      [&msg](system::system_error & se)
      {
        msg = sqlite3_mprintf("%s", se.what());

        if (se.code().category() == boost::sqlite::sqlite_category())
          return se.code().value();
        else
          return SQLITE_ERROR;
      },
      [](std::bad_alloc    &) { return SQLITE_NOMEM; },
      [](std::length_error &) { return SQLITE_TOOBIG; },
      [](std::out_of_range &) { return SQLITE_RANGE;},
      [&msg](std::logic_error &le)
      {
        msg = sqlite3_mprintf("%s", le.what());
        return SQLITE_MISUSE;
      },
      [&msg](std::exception & ex)
      {
        msg = sqlite3_mprintf("%s", ex.what());
        return SQLITE_ERROR;
      },
      [&msg](int id, error_info & ei)
      {
        msg = ei.release();
        return id;
      },
      [&msg](int id)
      {
        return id;
      },
      [&msg]
      {
        return SQLITE_ERROR;
      }
  );
}


template<typename Func>
int execute_function_return(Func && func) noexcept
{
  return leaf::try_handle_all(
      std::forward<Func>(func),
      [](system::system_error & se)
      {
        if (se.code().category() == boost::sqlite::sqlite_category())
          return se.code().value();
        else
          return SQLITE_ERROR;
      },
      [](std::bad_alloc    &) { return SQLITE_NOMEM; },
      [](std::length_error &) { return SQLITE_TOOBIG; },
      [](std::out_of_range &) { return SQLITE_RANGE;},
      [](std::logic_error &le){ return SQLITE_MISUSE; },
      [](std::exception & ex) { return SQLITE_ERROR; },
      [](int id) { return id; },
      []         { return SQLITE_ERROR; }
  );
}

}
BOOST_SQLITE_END_NAMESPACE


#define BOOST_SQLITE_TRY try

#define BOOST_SQLITE_CATCH_RESULT(ctx)                                             \
catch (boost::system::system_error & se)                                           \
{                                                                                  \
  if (se.code().category() == boost::sqlite::sqlite_category())                    \
    sqlite3_result_error_code(ctx, se.code().value());                             \
  sqlite3_result_error(ctx, se.what(), std::strlen(se.what()));                    \
}                                                                                  \
catch(std::bad_alloc    &) { sqlite3_result_error_nomem(ctx); }                    \
catch(std::length_error &) { sqlite3_result_error_toobig(ctx); }                   \
catch(std::out_of_range &) { sqlite3_result_error_code(ctx, SQLITE_RANGE);}        \
catch(std::logic_error &le)                                                        \
{                                                                                  \
  sqlite3_result_error_code(ctx, SQLITE_MISUSE);                                   \
  sqlite3_result_error(ctx, le.what(), std::strlen(le.what()));                    \
}                                                                                  \
catch(std::exception & ex)                                                         \
{                                                                                  \
  sqlite3_result_error(ctx, ex.what(), std::strlen(ex.what()));                    \
}                                                                                  \
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
