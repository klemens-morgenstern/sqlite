// Copyright (c) 2023 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_DETAIL_VTABLE_HPP
#define BOOST_SQLITE_DETAIL_VTABLE_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/detail/catch.hpp>
#include <boost/sqlite/vtable.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE
namespace detail
{
struct vtab_impl
{

template<typename Module>
static int connect(sqlite3 * db, void * pAux, int argc, const char * const * argv,
            sqlite3_vtab **ppVTab, char** errMsg)
{
  using table_type = typename Module::table_type;
  auto &impl = *static_cast<Module*>(pAux);
  BOOST_SQLITE_TRY
  {
    result<table_type> rtab = impl.connect(
        sqlite::connection(db, false),
        argc, argv);

    if (rtab.has_error())
      return extract_error(*errMsg, rtab);

    auto tab = make_unique<table_type>(std::move(*rtab));
    tab->db_ = db;
    auto code = sqlite3_declare_vtab(db, tab->declaration());
    if (code != SQLITE_OK)
      return code;

    sqlite::vtab::module_config cfg{db};
    auto r = tab->config(cfg);
    if (r.has_error())
      return extract_error(*errMsg, r);

    *ppVTab = tab.release();

    return SQLITE_OK;
  }
  BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(*errMsg)
}


template<typename Module>
static int create(sqlite3 * db, void * pAux, int argc, const char * const * argv,
           sqlite3_vtab **ppVTab, char** errMsg)
{
  using table_type = typename Module::table_type;
  auto &impl = *static_cast<Module*>(pAux);
  BOOST_SQLITE_TRY
  {
    result<table_type> rtab = impl.create(
        sqlite::connection(db, false),
        argc, argv);

    if (rtab.has_error())
      return extract_error(*errMsg, rtab);

    auto tab = make_unique<table_type>(std::move(*rtab));
    tab->db_ = db;

    auto code = sqlite3_declare_vtab(db, tab->declaration());
    if (code != SQLITE_OK)
      return code;

    sqlite::vtab::module_config mc{db};
    auto r = tab->config(mc);
    if (r.has_error())
      return extract_error(*errMsg, r);

    *ppVTab = tab.release();

    return SQLITE_OK;
  }
  BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(*errMsg)
}

template<typename Table>
static int disconnect(sqlite3_vtab * tab)
{
  BOOST_SQLITE_TRY
  {
    auto tb = static_cast<Table*>(tab);
    tb->~Table();
    sqlite3_free(tb);
    return SQLITE_OK;
  }
  BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(tab->zErrMsg);
}

template<typename Table>
static int destroy(sqlite3_vtab * tab)
{
  BOOST_SQLITE_TRY
  {
    auto tb = static_cast<Table*>(tab);
    auto res = tb->destroy();
    tb->~Table();
    sqlite3_free(tb);
    if (res.has_error())
      return std::move(res).error().code;
    return SQLITE_OK;
  }
  BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(tab->zErrMsg);
}

template<typename Module, typename Table>
static void assign_create(sqlite3_module & md, const Module & mod,
                   const sqlite::vtab::eponymous_module<Table> & base)
{
  md.xConnect = md.xCreate = &connect<Module>;
  md.xDisconnect = md.xDestroy = &disconnect<Table>;
  if (base.eponymous_only())
    md.xCreate = nullptr;
}

template<typename Module, typename Table>
static void assign_create(sqlite3_module & md, const Module & mod,
                   const sqlite::vtab::module<Table> & base)
{
  md.xConnect    = &connect<Module>;
  md.xDisconnect = &disconnect<Table>;
  md.xCreate     = &create<Module>;
  md.xDestroy    = &destroy<Table>;
}

template<typename Table>
static int open(sqlite3_vtab *pVTab, sqlite3_vtab_cursor **ppCursor)
{
  auto tab = static_cast<Table *>(pVTab);

  BOOST_SQLITE_TRY
  {
    auto res = tab->open();
    if (res.has_error())
      return extract_error(pVTab->zErrMsg, res);
    *ppCursor = new (memory_tag{}) typename Table::cursor_type(std::move(*res));
    return SQLITE_OK;
  }
  BOOST_SQLITE_CATCH_AND_RETURN();
}

template<typename Cursor>
static int close(sqlite3_vtab_cursor * cursor)
{
  auto p = static_cast<Cursor *>(cursor);

  BOOST_SQLITE_TRY
  {
    p->~Cursor();
  }
  BOOST_SQLITE_CATCH_AND_RETURN();

  sqlite3_free(p);
  return SQLITE_OK;

}

template<typename Table>
static int best_index(sqlite3_vtab *pVTab, sqlite3_index_info* info)
{
  BOOST_SQLITE_TRY
  {
    auto tb = static_cast<Table*>(pVTab);

    sqlite::vtab::index_info ii(tb->db_, info);
    auto r = tb->best_index(ii);
    if (r.has_error())
      return extract_error(pVTab->zErrMsg, r);

    return SQLITE_OK;
  }
  BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(pVTab->zErrMsg);
}


template<typename Cursor>
static int filter(sqlite3_vtab_cursor* pCursor,
           int idxNum, const char *idxStr,
           int argc, sqlite3_value **argv)
{
  BOOST_SQLITE_TRY
  {
    auto cr = static_cast<Cursor*>(pCursor);

    auto r = cr->filter(idxNum, idxStr,
                        boost::span<value>{reinterpret_cast<value*>(argv),
                                           static_cast<std::size_t>(argc)});
    if (r.has_error())
      return extract_error(pCursor->pVtab->zErrMsg, r);

    return SQLITE_OK;
  }
  BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(pCursor->pVtab->zErrMsg);
}


template<typename Cursor>
static int next(sqlite3_vtab_cursor* pCursor)
{
  BOOST_SQLITE_TRY
  {
    auto cr = static_cast<Cursor*>(pCursor);

    auto r = cr->next();
    if (r.has_error())
      return extract_error(pCursor->pVtab->zErrMsg, r);

    return SQLITE_OK;
  }
  BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(pCursor->pVtab->zErrMsg);
}


template<typename Cursor>
static int eof(sqlite3_vtab_cursor* pCursor)
{
  return static_cast<Cursor*>(pCursor)->eof() ? 1 : 0;
}

template<typename Cursor>
static auto column(sqlite3_vtab_cursor* pCursor,
                   sqlite3_context * ctx, int idx)
  -> typename std::enable_if<!std::is_void<typename Cursor::column_type>::value, int>::type
{
#if SQLITE_VERSION_NUMBER >= 3032000
  bool no_change = sqlite3_vtab_nochange(ctx) != 0;
#else
  bool no_change = false;
#endif
  auto cr = static_cast<Cursor*>(pCursor);
  execute_context_function(
      ctx,
      [&]{
        return cr->column(idx, no_change);
      });

  return  SQLITE_OK;
}


template<typename Cursor>
static auto column(sqlite3_vtab_cursor* pCursor,
                     sqlite3_context * ctx, int idx)
  -> typename std::enable_if<std::is_void<typename Cursor::column_type>::value, int>::type
{
#if SQLITE_VERSION_NUMBER >= 3032000
  bool no_change = sqlite3_vtab_nochange(ctx) != 0;
#else
  bool no_change = false;
#endif
  auto cr = static_cast<Cursor*>(pCursor);
  BOOST_SQLITE_TRY
  {
    cr->column(context<>{ctx}, idx, no_change);
  }
  BOOST_SQLITE_CATCH_RESULT(ctx);
  return  SQLITE_OK;
}

template<typename Cursor>
static int row_id(sqlite3_vtab_cursor* pCursor, sqlite3_int64 *pRowid)
{
  BOOST_SQLITE_TRY
  {
    auto cr = static_cast<Cursor*>(pCursor);

    auto r = cr->row_id();
    if (r.has_error())
      return extract_error(pCursor->pVtab->zErrMsg, r);
    *pRowid = *r;
    return SQLITE_OK;
  }
  BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(pCursor->pVtab->zErrMsg);
}

template<typename Table>
static int update(sqlite3_vtab * pVTab, int argc, sqlite3_value ** argv, sqlite3_int64 * pRowid)
{
  using table_type = Table;
  BOOST_SQLITE_TRY
  {
    auto & mod = *static_cast<table_type *>(pVTab);
    auto db = mod.db_;
      if (argc == 1 && sqlite3_value_type(argv[0]) != SQLITE_NULL)
    {
      auto r = mod.delete_(sqlite::value(*argv));
      if (r.has_error())
        return extract_error(pVTab->zErrMsg, r);
    }
    else if (argc > 1 && sqlite3_value_type(argv[0]) == SQLITE_NULL)
    {
      auto r = mod.insert(value{argv[1]},
                          boost::span<value>{reinterpret_cast<value *>(argv + 2),
                                             static_cast<std::size_t>(argc - 2)},
                                             sqlite3_vtab_on_conflict(db));
      if (r.has_error())
        return extract_error(pVTab->zErrMsg, r);
      *pRowid = r.value();
    }
    else if (argc > 1 && sqlite3_value_type(argv[0]) != SQLITE_NULL)
    {
      auto r = mod.update(sqlite::value(*argv), value{argv[1]}, // ID
                          boost::span<value>{reinterpret_cast<value *>(argv + 2),
                                             static_cast<std::size_t>(argc - 2)},
                                             sqlite3_vtab_on_conflict(db));

      if (r.has_error())
        return extract_error(pVTab->zErrMsg, r);
      *pRowid = r.value();
    }
    else
    {
      pVTab->zErrMsg = sqlite3_mprintf("Misuse of update api");
      return SQLITE_MISUSE;
    }

  }
  BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(pVTab->zErrMsg)
  return SQLITE_OK;
}

template<typename Module>
static void assign_update(sqlite3_module & md, const Module & mod,
                   std::true_type /* modifiable */)
{
  md.xUpdate = &update<typename Module::table_type>;
}

template<typename Module>
static void assign_update(sqlite3_module & md, const Module & mod,
                   std::false_type /* modifiable */)
{
}

template<typename Table>
static int begin(sqlite3_vtab* pVTab)
{
  BOOST_SQLITE_TRY
  {
    auto cr = static_cast<Table*>(pVTab);

    auto r = cr->begin();
    if (r.has_error())
      return extract_error(pVTab->zErrMsg, r);

    return SQLITE_OK;
  }
  BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(pVTab->zErrMsg);
}

template<typename Table>
static int sync(sqlite3_vtab* pVTab)
{
  BOOST_SQLITE_TRY
  {
    auto cr = static_cast<Table*>(pVTab);

    auto r = cr->sync();
    if (r.has_error())
      return extract_error(pVTab->zErrMsg, r);

    return SQLITE_OK;
  }
  BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(pVTab->zErrMsg);
}

template<typename Table>
static int commit(sqlite3_vtab* pVTab)
{
  BOOST_SQLITE_TRY
  {
    auto cr = static_cast<Table*>(pVTab);

    auto r = cr->commit();
    if (r.has_error())
      return extract_error(pVTab->zErrMsg, r);

    return SQLITE_OK;
  }
  BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(pVTab->zErrMsg);
}

template<typename Table>
static int rollback(sqlite3_vtab* pVTab)
{
  BOOST_SQLITE_TRY
  {
    auto cr = static_cast<Table*>(pVTab);

    auto r = cr->rollback();
    if (r.has_error())
      return extract_error(pVTab->zErrMsg, r);

    return SQLITE_OK;
  }
  BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(pVTab->zErrMsg);
}

template<typename Module>
static void assign_transaction(sqlite3_module & md, const Module & mod,
                        std::false_type /* modifiable */)
{
}

template<typename Module>
static void assign_transaction(sqlite3_module & md, const Module & mod,
                        std::true_type /* modifiable */)
{
  md.xBegin    = &begin   <typename Module::table_type>;
  md.xSync     = &sync    <typename Module::table_type>;
  md.xCommit   = &commit  <typename Module::table_type>;
  md.xRollback = &rollback<typename Module::table_type>;
}

template<typename Table>
static int find_function(sqlite3_vtab *pVtab, int nArg, const char *zName,
                  void (**pxFunc)(sqlite3_context*,int,sqlite3_value**),
                  void **ppArg)
{
  BOOST_SQLITE_TRY
  {
    auto cr = static_cast<Table*>(pVtab);

    auto r = cr->find_function(
        nArg, zName, sqlite::vtab::function_setter(pxFunc, ppArg));
    if (r.has_error())
      return extract_error(pVtab->zErrMsg, r);

    return SQLITE_OK;
  }
  BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(pVtab->zErrMsg);
}

template<typename Module>
static void assign_find_function(sqlite3_module & md, const Module & mod,
                          std::false_type /* overloadable */)
{
}

template<typename Module>
static void assign_find_function(sqlite3_module & md, const Module & mod,
                          std::true_type /* overloadable */)
{
  md.xFindFunction = &find_function<typename Module::table_type>;
}

template<typename Table>
static int rename(sqlite3_vtab* pVTab, const char * name)
{
  BOOST_SQLITE_TRY
  {
    auto cr = static_cast<Table*>(pVTab);

    auto r = cr->rename(name);
    if (r.has_error())
      return extract_error(pVTab->zErrMsg, r);

    return SQLITE_OK;
  }
  BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(pVTab->zErrMsg);
}

template<typename Module>
static void assign_rename(sqlite3_module & md, const Module & mod,
                   std::false_type /* renamable */)
{
}

template<typename Module>
static void assign_rename(sqlite3_module & md, const Module & mod,
                   std::true_type /* renamable */)
{
  md.xRename = &rename<typename Module::table_type>;
}
#if SQLITE_VERSION_NUMBER >= 3007007

template<typename Table>
static int savepoint(sqlite3_vtab* pVTab, int i)
{
  BOOST_SQLITE_TRY
  {
    auto cr = static_cast<Table*>(pVTab);

    auto r = cr->savepoint(i);
    if (r.has_error())
      return extract_error(pVTab->zErrMsg, r);

    return SQLITE_OK;
  }
  BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(pVTab->zErrMsg);
}

template<typename Table>
static int release(sqlite3_vtab* pVTab, int i)
{
  BOOST_SQLITE_TRY
  {
    auto cr = static_cast<Table*>(pVTab);

    auto r = cr->release(i);
    if (r.has_error())
      return extract_error(pVTab->zErrMsg, r);

    return SQLITE_OK;
  }
  BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(pVTab->zErrMsg);
}

template<typename Table>
static int rollback_to(sqlite3_vtab* pVTab, int i)
{
  BOOST_SQLITE_TRY
  {
    auto cr = static_cast<Table*>(pVTab);

    auto r = cr->rollback_to(i);
    if (r.has_error())
      return extract_error(pVTab->zErrMsg, r);

    return SQLITE_OK;
  }
  BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(pVTab->zErrMsg);
}

template<typename Module>
static void assign_recursive_transaction(sqlite3_module & md, const Module & mod,
                                  std::false_type /* recursive_transaction */)
{
}

template<typename Module>
static void assign_recursive_transaction(sqlite3_module & md, const Module & mod,
                                  std::true_type /* recursive_transaction */)
{
  md.xSavepoint  = &savepoint  <typename Module::table_type>;
  md.xRelease    = &release    <typename Module::table_type>;
  md.xRollbackTo = &rollback_to<typename Module::table_type>;
}

#endif

#if SQLITE_VERSION_NUMBER >= 3026000

template<typename Table>
static void assign_shadow_name(sqlite3_module & md, const sqlite::vtab::module<Table> &) {}

template<typename Table>
static void assign_shadow_name(sqlite3_module & md, const sqlite::vtab::eponymous_module<Table> &) {}

template<typename Module,
    bool (*Func)(const char *) = &Module::shadow_name>
static void assign_shadow_name(sqlite3_module & md, const Module & mod)
{
  md.xShadowName = +[](const char * name){return Func(name) != 0;};
}

};

#endif

template<typename Module>
const sqlite3_module make_module(const Module & mod)
{
  sqlite3_module md;
  std::memset(&md, 0, sizeof(sqlite3_module));

#if SQLITE_VERSION_NUMBER < 3007007
  md.iVersion = 1;
#elif SQLITE_VERSION_NUMBER < 3026000
  md.iVersion = 2;
#else
  md.iVersion = 3;
#endif
  using table_type = typename Module::table_type;
  using cursor_type = typename table_type::cursor_type;
  vtab_impl::assign_create(md, mod, mod);
  md.xBestIndex = &vtab_impl::best_index<table_type>;
  md.xOpen      = &vtab_impl::open      <table_type>;
  md.xClose     = &vtab_impl::close     <cursor_type>;
  md.xFilter    = &vtab_impl::filter    <cursor_type>;
  md.xNext      = &vtab_impl::next      <cursor_type>;
  md.xEof       = &vtab_impl::eof       <cursor_type>;
  md.xColumn    = &vtab_impl::column    <cursor_type>;
  md.xRowid     = &vtab_impl::row_id    <cursor_type>;
  vtab_impl::assign_update       (md, mod, std::is_base_of<sqlite::vtab::modifiable,            table_type>{});
  vtab_impl::assign_transaction  (md, mod, std::is_base_of<sqlite::vtab::transaction,           table_type>{});
  vtab_impl::assign_find_function(md, mod, std::is_base_of<sqlite::vtab::overload_functions,    table_type>{});
  vtab_impl::assign_rename       (md, mod, std::is_base_of<sqlite::vtab::renamable,             table_type>{});
#if SQLITE_VERSION_NUMBER >= 3007007
  vtab_impl::assign_recursive_transaction(md, mod, std::is_base_of<sqlite::vtab::recursive_transaction, table_type>{});
#endif
#if SQLITE_VERSION_NUMBER >= 3026000
  vtab_impl::assign_shadow_name(md, mod);
#endif
  return md;
}

}

BOOST_SQLITE_END_NAMESPACE

#endif //BOOST_SQLITE_DETAIL_VTABLE_HPP
