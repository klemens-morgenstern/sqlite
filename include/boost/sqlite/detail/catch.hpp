//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_DETAIL_CATCH_HPP
#define BOOST_SQLITE_DETAIL_CATCH_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/detail/exception.hpp>
#include <boost/sqlite/error.hpp>
#include <boost/sqlite/result.hpp>

#include <boost/system/system_error.hpp>

#include <stdexcept>


BOOST_SQLITE_BEGIN_NAMESPACE
namespace detail
{

template<typename Func, typename ... Args>
void execute_context_function_impl(std::false_type /* is_void */,
                                   sqlite3_context * ctx,
                                   Func && func, Args && ... args)
    noexcept(noexcept(std::forward<Func>(func)(std::forward<Args>(args)...)))
{
  set_result(ctx, std::forward<Func>(func)(std::forward<Args>(args)...));
}

template<typename Func, typename ... Args>
void execute_context_function_impl(std::true_type  /* is_void */,
                                   sqlite3_context * ctx,
                                   Func && func, Args && ... args)
    noexcept(noexcept(std::forward<Func>(func)(std::forward<Args>(args)...)))
{
  std::forward<Func>(func)(std::forward<Args>(args)...);
}

template<typename T>
int extract_error(char * &errMsg, result<T> & res)
{
  error err = std::move(res).error();
  if (err.info)
    errMsg = err.info.release();
  return err.code;
}


template<typename Func, typename ... Args>
void execute_context_function(sqlite3_context * ctx,
                              Func && func, Args && ... args) noexcept
{
  using return_type = decltype(func(std::forward<Args>(args)...));
  using is_result_type = is_result_type<return_type>;
  using result_type = typename std::conditional<
                          is_result_type::value,
                          return_type,
                          result<return_type>>::type::value_type;
#if !defined(BOOST_NO_EXCEPTIONS)
  try
  {
#endif
    execute_context_function_impl(std::is_void<return_type>{}, ctx,
                                  std::forward<Func>(func),
                                  std::forward<Args>(args)...);
#if !defined(BOOST_NO_EXCEPTIONS)
  }
  catch(boost::system::system_error & se)
  {
    if (se.code().category() == boost::sqlite::sqlite_category())
      sqlite3_result_error_code(ctx, se.code().value());
    const auto msg = boost::sqlite::detail::get_message(se);
    if (!msg.empty())
      sqlite3_result_error(ctx, msg.data(), msg.size());
  }
  catch(std::bad_alloc    &) { sqlite3_result_error_nomem(ctx); }
  catch(std::length_error &) { sqlite3_result_error_toobig(ctx); }
  catch(std::out_of_range &) { sqlite3_result_error_code(ctx, SQLITE_RANGE);}
  catch(std::logic_error &le)
  {
    sqlite3_result_error_code(ctx, SQLITE_MISUSE);
    sqlite3_result_error(ctx, le.what(), std::strlen(le.what()));
  }
  catch(std::exception & ex)
  {
    sqlite3_result_error(ctx, ex.what(), std::strlen(ex.what()));
  }
  catch(...) {sqlite3_result_error_code(ctx, SQLITE_ERROR); }
#endif
}

}
BOOST_SQLITE_END_NAMESPACE

#if defined(BOOST_NO_EXCEPTIONS)
#define BOOST_SQLITE_TRY
#define BOOST_SQLITE_CATCH_RESULT(ctx)
#define BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(msg)
#define BOOST_SQLITE_CATCH_AND_RETURN()

#else

#define BOOST_SQLITE_TRY try
#define BOOST_SQLITE_CATCH_RESULT(ctx)                                       \
catch(boost::system::system_error & se)                                      \
{                                                                            \
  if (se.code().category() == boost::sqlite::sqlite_category())              \
    sqlite3_result_error_code(ctx, se.code().value());                       \
  const auto msg = boost::sqlite::detail::get_message(se);                   \
  if (!msg.empty())                                                          \
    sqlite3_result_error(ctx, msg.data(), msg.size());                       \
}                                                                            \
catch(std::bad_alloc    &) { sqlite3_result_error_nomem(ctx); }              \
catch(std::length_error &) { sqlite3_result_error_toobig(ctx); }             \
catch(std::out_of_range &) { sqlite3_result_error_code(ctx, SQLITE_RANGE);}  \
catch(std::logic_error &le)                                                  \
{                                                                            \
  sqlite3_result_error_code(ctx, SQLITE_MISUSE);                             \
  sqlite3_result_error(ctx, le.what(), std::strlen(le.what()));              \
}                                                                            \
catch(std::exception & ex)                                                   \
{                                                                            \
  sqlite3_result_error(ctx, ex.what(), std::strlen(ex.what()));              \
}                                                                            \
catch(...) {sqlite3_result_error_code(ctx, SQLITE_ERROR); }


#define BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(msg)                        \
catch (boost::system::system_error & se)                                     \
{                                                                            \
  auto code = SQLITE_ERROR;                                                  \
  if (se.code().category() == boost::sqlite::sqlite_category())              \
    code = se.code().value();                                                \
  const auto pre = boost::sqlite::detail::get_message(se);                   \
  msg = boost::sqlite::error_info(pre).release();                            \
  return code;                                                               \
}                                                                            \
catch(std::bad_alloc    &) { return SQLITE_NOMEM; }                          \
catch(std::length_error &) { return SQLITE_TOOBIG; }                         \
catch(std::out_of_range &) { return SQLITE_RANGE;}                           \
catch(std::logic_error &le)                                                  \
{                                                                            \
    msg = boost::sqlite::error_info(le.what()).release();                    \
    return SQLITE_MISUSE;                                                    \
}                                                                            \
catch(std::exception & ex)                                                   \
{                                                                            \
    msg = boost::sqlite::error_info(ex.what()).release();                    \
    return SQLITE_ERROR;                                                     \
}                                                                            \
catch(...) {return SQLITE_ERROR; }                                           \


#define BOOST_SQLITE_CATCH_AND_RETURN()                                      \
catch (boost::system::system_error & se)                                     \
{                                                                            \
  auto code = SQLITE_ERROR;                                                  \
  if (se.code().category() == boost::sqlite::sqlite_category())              \
    code = se.code().value();                                                \
  return code;                                                               \
}                                                                            \
catch(std::bad_alloc    &) { return SQLITE_NOMEM; }                          \
catch(std::length_error &) { return SQLITE_TOOBIG; }                         \
catch(std::out_of_range &) { return SQLITE_RANGE;}                           \
catch(std::logic_error  &) { return SQLITE_MISUSE;}                          \
catch(...) { return SQLITE_ERROR; }                                          \

#endif

#endif //BOOST_SQLITE_DETAIL_CATCH_HPP
