//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_VTABLE_HPP
#define BOOST_SQLITE_VTABLE_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/function.hpp>

#include <boost/core/span.hpp>
#include <boost/core/demangle.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE


/// Helper type to set a function through the xFindFunction callback
struct vtab_function_setter
{
    /** Set the function
     *
     * @tparam Func The function type (either a lambda by ref or a pointer by copy)
     * @param func The function to be used
     *
     * The function can either take a single argument, a `span<sqlite::value, N>`
     * for scalar functions,
     * or a `context<Args...>` as first, and the span as second for aggegrate functions.
     *
     */
    template<typename Func>
    void set(Func & func)
    {
      set_impl(func, static_cast<callable_traits::return_type_t<Func>*>(nullptr),
                     static_cast<callable_traits::args_t<Func>*>(nullptr));
    }

    template<typename ... Args, std::size_t Extent>
    void set(void(* ptr)(context<Args...>, span<value, Extent>))
    {
      *ppArg_ = reinterpret_cast<void*>(ptr);
      *pxFunc_ =
          +[](sqlite3_context* ctx, int len, sqlite3_value** args)
          {
            auto cc = context<Args...>(ctx);
            auto aa =  reinterpret_cast<value*>(args);
            auto &f = *reinterpret_cast<void(*)(context<Args...>, span<value, Extent>)>(sqlite3_user_data(ctx));

            try
            {
                f(cc, boost::span<value, Extent>{aa, static_cast<std::size_t>(len)});
            }
            BOOST_SQLITE_CATCH_RESULT(ctx);
          };
    }

    template<typename T, typename ... Args,  std::size_t Extent>
    void set(T(* ptr)(context<Args...>, span<value, Extent>))
    {
      *ppArg_ = reinterpret_cast<void*>(ptr);
      *pxFunc_ =
          +[](sqlite3_context* ctx, int len, sqlite3_value** args)
          {
            auto cc = context<Args...>(ctx);
            auto aa =  reinterpret_cast<value*>(args);
            auto &f = *reinterpret_cast<T(*)(context<Args...>, span<value, Extent>)>(sqlite3_user_data(ctx));

            try
            {
                set_result(ctx, f(cc, boost::span<value, Extent>{aa, static_cast<std::size_t>(len)}));
            }
            BOOST_SQLITE_CATCH_RESULT(ctx);
          };
    }

    template<std::size_t Extent>
    void set(void(* ptr)(span<value, Extent>))
    {
      *ppArg_ = reinterpret_cast<void*>(ptr);
      *pxFunc_ =
          +[](sqlite3_context* ctx, int len, sqlite3_value** args)
          {
            auto aa =  reinterpret_cast<value*>(args);
            auto &f = *reinterpret_cast<void(*)(span<value, Extent>)>(sqlite3_user_data(ctx));

            try
            {
                f(boost::span<value, Extent>{aa, static_cast<std::size_t>(len)});
            }
            BOOST_SQLITE_CATCH_RESULT(ctx);
          };
    }

    template<typename T, std::size_t Extent>
    void set(T(* ptr)(span<value, Extent>))
    {
      *ppArg_ = reinterpret_cast<void*>(ptr);
      *pxFunc_ =
          +[](sqlite3_context* ctx, int len, sqlite3_value** args)
          {
            auto aa =  reinterpret_cast<value*>(args);
            auto &f = *reinterpret_cast<T(*)(span<value, Extent>)>(sqlite3_user_data(ctx));

            try
            {
                set_result(ctx, f(boost::span<value, Extent>{aa, static_cast<std::size_t>(len)}));
            }
            BOOST_SQLITE_CATCH_RESULT(ctx);
          };
    }

    explicit vtab_function_setter(void (**pxFunc)(sqlite3_context*,int,sqlite3_value**),
                                  void **ppArg) : pxFunc_(pxFunc), ppArg_(ppArg) {}
  private:

    template<typename Func, typename ... Args, std::size_t Extent>
    void set_impl(Func & func, void *, std::tuple<context<Args...>, span<value, Extent>>)
    {
      *ppArg_ = &func;
      *pxFunc_ =
          +[](sqlite3_context* ctx, int len, sqlite3_value** args)
          {
            auto cc = context<Args...>(ctx);
            auto aa =  reinterpret_cast<value*>(args);
            auto &f = *reinterpret_cast<Func*>(sqlite3_user_data(ctx));

            try
            {
                f(cc, boost::span<value, Extent>{aa, static_cast<std::size_t>(len)});
            }
            BOOST_SQLITE_CATCH_RESULT(ctx);
          };
    }

    template<typename Func, typename T, typename ... Args,  std::size_t Extent>
    void set_impl(Func & func, T *, std::tuple<context<Args...>, span<value, Extent>>)
    {
      *ppArg_ = &func;
      *pxFunc_ =
          +[](sqlite3_context* ctx, int len, sqlite3_value** args)
          {
            auto cc = context<Args...>(ctx);
            auto aa =  reinterpret_cast<value*>(args);
            auto &f = *reinterpret_cast<Func*>(sqlite3_user_data(ctx));

            try
            {
                set_result(ctx, f(cc, boost::span<value, Extent>{aa, static_cast<std::size_t>(len)}));
            }
            BOOST_SQLITE_CATCH_RESULT(ctx);
          };
    }

    template<typename Func, std::size_t Extent>
    void set_impl(Func & func, void* , std::tuple<span<value, Extent>> * )
    {
      *ppArg_ = &func;
      *pxFunc_ =
          +[](sqlite3_context* ctx, int len, sqlite3_value** args)
          {
            auto aa =  reinterpret_cast<value*>(args);
            auto &f = *reinterpret_cast<Func *>(sqlite3_user_data(ctx));

            try
            {
                f(boost::span<value, Extent>{aa, static_cast<std::size_t>(len)});
            }
            BOOST_SQLITE_CATCH_RESULT(ctx);
          };
    }

    template<typename Func, typename T, std::size_t Extent>
    void set_impl(Func & func, T* , std::tuple<span<value, Extent>> * )
    {
      *ppArg_ = &func;
      *pxFunc_ =
          +[](sqlite3_context* ctx, int len, sqlite3_value** args)
          {
            auto aa =  reinterpret_cast<value*>(args);
            auto &f = *reinterpret_cast<Func *>(sqlite3_user_data(ctx));

            try
            {
                set_result(ctx, f(boost::span<value, Extent>{aa, static_cast<std::size_t>(len)}));
            }
            BOOST_SQLITE_CATCH_RESULT(ctx);
          };
    }

    void (**pxFunc_)(sqlite3_context*,int,sqlite3_value**);
    void **ppArg_;
};

#if SQLITE_VERSION_NUMBER > 3038000
/** @brief Utility function that can be used in `xFilter` for the `in` operator.
    @see https://www.sqlite.org/capi3ref.html#sqlite3_vtab_in_first
    @ingroup reference
    @Note requires sqlite version >= 3.38
*/
struct vtab_in
{
    struct iterator
    {
        iterator() = default;
        explicit iterator(sqlite3_value * value ) : value_(value)
        {
            if (value == nullptr)
                return ;
            auto res = sqlite3_vtab_in_first(value, &out_.handle());
            if (res != SQLITE_OK)
            {
              system::error_code ec;
              BOOST_SQLITE_ASSIGN_EC(ec, res);
              detail::throw_error_code(ec, ec.location());
            }
        }

      iterator & operator++()
        {
          auto res = sqlite3_vtab_in_next(value_, &out_.handle());
          if (res != SQLITE_OK)
          {
            system::error_code ec;
            BOOST_SQLITE_ASSIGN_EC(ec, res);
            detail::throw_error_code(ec, ec.location());
          }
        }

      iterator operator++(int)
        {
            auto last = *this;
            ++(*this);
            return last;
        }


        const value & operator*() const
        {
            return out_;
        }

        const value * operator->() const
        {
            return &out_;
        }

        bool operator==(const iterator& other) const
        {
            return value_ == other.value_
                && out_.handle() == other.out_.handle();
        }

        bool operator!=(const iterator& other) const
        {
            return value_ != other.value_
                || out_.handle()   != other.out_.handle();
        }


      private:
        sqlite3_value * value_{nullptr};
        value out_{nullptr};
    };

    /// Returns a forward iterator to the `in` sequence for an `in` constraint pointing to the begin.
    iterator begin() {return iterator(out_);}
    /// Returns a forward iterator to the `in` sequence for an `in` constraint pointing to the end.
    iterator end() {return iterator();}

    explicit vtab_in(sqlite3_value * out) : out_(out) {}
 private:
    sqlite3_value * out_{nullptr};
};

#endif

namespace detail
{

namespace vtab
{

template<std::size_t I>
struct rank : rank<I - 1> {};

template<>
struct rank<0u> {};

template<typename Impl>
constexpr bool has_create_impl(rank<0>) { return false; };

template<typename Impl, typename Func = decltype(&Impl::create)>
constexpr bool has_create_impl(rank<1>) { return true; };

template<typename Impl>
using has_create = std::integral_constant<bool, has_create_impl<Impl>(rank<1>())>;

template<typename Impl>
using eponymous = std::integral_constant<bool, !has_create_impl<Impl>(rank<1>())>;

template<typename Impl>
constexpr bool eponymous_only_impl(rank<0>) { return false; };

template<typename Impl, bool EO = Impl::eponymous_only>
constexpr bool eponymous_only_impl(rank<1>) { return EO; };

template<typename Impl>
using eponymous_only = std::integral_constant<bool, eponymous_only_impl<Impl>(rank<1>())>;

template<typename Impl>
constexpr bool has_delete(rank<0>) { return false; };

template<typename Impl, typename = decltype(&Impl::delete_)>
constexpr bool has_delete(rank<1>) { return true; };

template<typename Impl>
using read_only = std::integral_constant<bool, !has_delete<Impl>(rank<1>())>;

template<typename Impl>
constexpr bool has_commit(rank<0>) { return false; };

template<typename Impl, typename = decltype(&Impl::commit)>
constexpr bool has_commit(rank<1>) { return true; };

template<typename Impl>
using has_transactions = std::integral_constant<bool, has_commit<Impl>(rank<1>())>;

template<typename Impl>
constexpr bool has_savepoint(rank<0>) { return false; };

template<typename Impl, typename = decltype(&Impl::savepoint)>
constexpr bool has_savepoint(rank<1>) { return true; };

template<typename Impl>
using has_recursive_transactions = std::integral_constant<bool, has_savepoint<Impl>(rank<1>())>;

template<typename Impl>
constexpr bool had_find_function_impl(rank<0>) { return false; };

template<typename Impl, typename = decltype(&Impl::find_function)>
constexpr bool had_find_function_impl(rank<1>) { return true; };

template<typename Impl>
using had_find_function = std::integral_constant<bool, had_find_function_impl<Impl>(rank<1>())>;

template<typename Impl>
constexpr void create_type_impl(rank<0>);

template<typename Impl, typename Func = decltype(&Impl::create)>
constexpr auto create_type_impl(rank<1>) -> boost::callable_traits::return_type_t<Func>;

template<typename Impl>
using create_type = decltype(create_type_impl<Impl>(rank<1>()));

template<typename Impl>
using connect_type = boost::callable_traits::return_type_t<decltype(&Impl::connect)>;

template<typename Table>
using cursor_type = boost::callable_traits::return_type_t<decltype(&Table::open)>;

template<typename Impl>
constexpr bool has_best_index_impl(rank<0>) { return false; };

template<typename Impl, typename Func = decltype(&Impl::best_index)>
constexpr bool has_best_index_impl(rank<1>) { return true; };

template<typename Impl>
using has_best_index = std::integral_constant<bool, has_best_index_impl<Impl>(rank<1>())>;


template<typename Base, typename Impl>
struct module_impl_helper : Base, Impl
{
  template<typename ... Args>
  module_impl_helper(Args && ... args) : Impl(std::forward<Args>(args)...) {}
  Impl & get_impl() {return *this;}
};


template<typename Impl, typename Base>
Impl & get_module(Base * impl) noexcept
{
  return static_cast<module_impl_helper<Base, Impl>*>(impl)->get_impl();
}

template<typename Impl, typename Base>
void delete_module(Base * impl) noexcept
{
  delete static_cast<module_impl_helper<Base, Impl>*>(impl);
}

template<typename BasePtr, typename ReturnType>
auto make_module(BasePtr &res, ReturnType && result)
    -> typename std::decay<ReturnType>::type *
{
    using impl_t = module_impl_helper<
        typename std::remove_pointer<BasePtr>::type,
        typename std::decay<ReturnType>::type>;
    std::unique_ptr<impl_t> ip{new impl_t(std::forward<ReturnType>(result))};
    auto p = ip.get();
    res = ip.release();
    return &p->get_impl();
}


template<typename Impl>
struct vtable_helper
{
    using implementation_type        = Impl;
    using eponymous_only             = vtab::eponymous_only<implementation_type>;
    using eponymous                  = vtab::eponymous<implementation_type>;
    using table_type                 = vtab::connect_type<implementation_type>;
    using read_only                  = vtab::read_only<table_type>;
    using cursor_type                = vtab::cursor_type<table_type>;
    using has_best_index             = vtab::has_best_index<table_type>;
    using has_transactions           = vtab::has_transactions<table_type>;
    using has_recursive_transactions = vtab::has_recursive_transactions<table_type>;
    using had_find_function          = vtab::had_find_function<table_type>;

    // -------------------------- create/destroy -------------------------- //
    static int create_impl(sqlite3 * db, void * pAux, int argc, const char * const * argv,
                           sqlite3_vtab **ppVTab, char** errMsg)
    {
        auto &impl = *static_cast<Impl*>(pAux);
        try
        {
            auto mod = make_module(*ppVTab, static_cast<table_type>(impl.create(argc, argv)));
            sqlite3_declare_vtab(db, mod->declaration());
            return SQLITE_OK;
        }
        BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(*errMsg)
    }

    static int connect_impl(sqlite3 * db, void * pAux, int argc, const char * const * argv,
                            sqlite3_vtab **ppVTab, char** errMsg)
    {
        auto &impl = *static_cast<Impl*>(pAux);
        try
        {
            auto mod = make_module(*ppVTab, impl.connect(argc, argv));
            sqlite3_declare_vtab(db, mod->declaration());
            return SQLITE_OK;
        }
        BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(*errMsg)
    }


    static int disconnect_impl(sqlite3_vtab * pvTab)
    {
      delete_module<table_type>(pvTab);
      return SQLITE_OK;
    }
    static int destroy_impl(sqlite3_vtab * pvTab)
    {
      get_module<table_type>(pvTab).destroy();
      delete_module<table_type>(pvTab);
      return SQLITE_OK;
    }

    using x_create_type = decltype(&create_impl);

    constexpr static
    x_create_type create(std::true_type /* eponymous */ ,
                         std::true_type /* eponymous_only */ ) { return nullptr; }

    constexpr static
    x_create_type create(std::true_type  /* eponymous */,
                         std::false_type /* eponymous_only */) { return &connect_impl; }

    constexpr static
    x_create_type create(std::false_type /* eponymous */,
                         std::false_type /* eponymous_only */)
    {
        return &create_impl;
    }

    using x_destroy_type = decltype(&destroy_impl);

    constexpr static
    x_destroy_type destroy(std::true_type /* eponymous */,
                           std::true_type /* eponymous_only */) { return nullptr; }

    constexpr static
    x_destroy_type destroy(std::true_type  /* eponymous */,
                           std::false_type /* eponymous_only */) { return &disconnect_impl; }

    constexpr static
    x_destroy_type destroy(std::false_type /* eponymous */,
                           std::false_type /* eponymous_only */)
    {
        return &destroy_impl;
    }

    // -------------------------- find_index -------------------------- //
    static int best_index_impl(sqlite3_vtab *pvTab, sqlite3_index_info* info)
    {
        try
        {
            get_module<table_type>(pvTab).best_index(info);
            return SQLITE_OK;
        }
        BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(pvTab->zErrMsg);
    }

    static int best_index_noop(sqlite3_vtab *pVTab, sqlite3_index_info* info) { return SQLITE_OK; }

    constexpr static auto best_index(std::true_type ) -> int (*)(sqlite3_vtab*, sqlite3_index_info*)
    {
        return &best_index_impl;
    }
    constexpr static auto best_index(std::false_type) -> int (*)(sqlite3_vtab*, sqlite3_index_info*)
    {
        return &best_index_noop;
    }

    static int open_impl(sqlite3_vtab *pVTab, sqlite3_vtab_cursor **ppCursor)
    {
        auto &impl = get_module<table_type>(pVTab);
        try
        {
            make_module(*ppCursor, impl.open());
            return SQLITE_OK;
        }
        BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(pVTab->zErrMsg)
    }

    static int close_impl(sqlite3_vtab_cursor *ppCursor)
    {
        try
        {
            delete_module<cursor_type>(ppCursor);
            return SQLITE_OK;
        }
        BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(ppCursor->pVtab->zErrMsg)
    }

    static int next_impl(sqlite3_vtab_cursor *ppCursor)
    {
        try
        {
            get_module<cursor_type>(ppCursor).next();
            return SQLITE_OK;
        }
        BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(ppCursor->pVtab->zErrMsg)
    }

    static int eof_impl(sqlite3_vtab_cursor *ppCursor)
    {
        auto & mod = get_module<cursor_type>(ppCursor);
        static_assert(noexcept(mod.eof()), "eof must be noexcept");
        return mod.eof() ? 1 : 0;
    }

    static int filter_impl(sqlite3_vtab_cursor* pCursor,
                           int idxNum, const char *idxStr,
                           int argc, sqlite3_value **argv)
    {
        try
        {
            get_module<cursor_type>(pCursor)
                .filter(idxNum, idxStr,
                        boost::span<value>{reinterpret_cast<value*>(argv),
                                           static_cast<std::size_t>(argc)} );
            return SQLITE_OK;
        }
        BOOST_SQLITE_CATCH_AND_RETURN();
        return SQLITE_OK;
    }

    static int filter_noop(sqlite3_vtab_cursor* pCursor,
                           int idxNum, const char *idxStr,
                           int argc, sqlite3_value **argv) { return SQLITE_OK; }

    static auto filter(std::true_type ) -> decltype(sqlite3_module{}.xFilter)
    {
        return &filter_impl;
    }
    static auto filter(std::false_type) -> decltype(sqlite3_module{}.xFilter)
    {
        return &filter_noop;
    }

    static void column_impl_1(sqlite3_vtab_cursor *pCursor, sqlite3_context* ctx, int idx,
                              context<> * /* first_type */) noexcept(
                                    noexcept(get_module<cursor_type>(static_cast<sqlite3_vtab_cursor*>(nullptr))
                                        .column(context<>(nullptr), int(), true)))
    {
#if SQLITE_VERSION_NUMBER >= 3032000
        bool no_change = sqlite3_vtab_nochange(ctx) != 0;
#else
        bool no_change = false;
#endif
        get_module<cursor_type>(pCursor).column(context<>(ctx), idx, no_change);
    }

    static void column_impl_1(sqlite3_vtab_cursor *pCursor, sqlite3_context* ctx, int idx,
                              int * /* first_type */)
    {
#if SQLITE_VERSION_NUMBER >= 3032000
        bool no_change = sqlite3_vtab_nochange(ctx) != 0;
#else
        bool no_change = false;
#endif
        set_result(ctx, get_module<cursor_type>(pCursor).column(idx, no_change));
    }

    static int column_impl(sqlite3_vtab_cursor *pCursor, sqlite3_context* ctx, int idx)
    {
        using first_type = typename std::tuple_element<1u, callable_traits::args_t<decltype(&cursor_type::column)>>::type;
        try
        {
            column_impl_1(pCursor, ctx, idx, static_cast<first_type*>(nullptr));
        }
        BOOST_SQLITE_CATCH_RESULT(ctx);
        return SQLITE_OK;
    }

    static int row_id_impl(sqlite3_vtab_cursor *pCursor, sqlite3_int64 *pRowid)
    {
        try
        {
            *pRowid = get_module<cursor_type>(pCursor).row_id();
        }
        BOOST_SQLITE_CATCH_AND_RETURN();
        return SQLITE_OK;
    }

    static int update_impl(sqlite3_vtab * pVTab, int argc, sqlite3_value ** argv, sqlite3_int64 * pRowid)
    {
        try
        {
            auto & mod = get_module<table_type>(pVTab);
            if (argc == 1 && sqlite3_value_type(argv[0]) != SQLITE_NULL)
                mod.delete_(sqlite::value(*argv));
            else if (argc > 1 && sqlite3_value_type(argv[0]) == SQLITE_NULL)
                *pRowid = mod.insert(value{argv[1]},
                                     boost::span<value>{reinterpret_cast<value*>(argv + 2),
                                                        static_cast<std::size_t>(argc - 2)});
            else if (argc > 1 && sqlite3_value_type(argv[0]) != SQLITE_NULL)
              *pRowid = mod.update(sqlite::value(*argv), value{argv[1]}, // ID
                                   boost::span<value>{reinterpret_cast<value*>(argv + 2),
                                                      static_cast<std::size_t>(argc - 2)});
            else
            {
              pVTab->zErrMsg = sqlite3_mprintf("Misuse of update api");
              return SQLITE_MISUSE;
            }

        }
        BOOST_SQLITE_CATCH_ASSIGN_STR_AND_RETURN(pVTab->zErrMsg)
        return SQLITE_OK;
    }

    constexpr static auto update(std::false_type /* read_only */) -> decltype(sqlite3_module{}.xUpdate)
    {
        return &update_impl;
    }
    constexpr static auto update(std::true_type /* read_only */) -> decltype(sqlite3_module{}.xUpdate)
    {
        return nullptr;
    }
  
    static int begin_impl(sqlite3_vtab *pVTab)
    {
        try
        {
            get_module<table_type>(pVTab).begin();
            return SQLITE_OK;
        }
        BOOST_SQLITE_CATCH_AND_RETURN();
    }

    constexpr static auto begin(std::true_type /* has_transaction */) -> decltype(sqlite3_module{}.xBegin)
    {
       return &begin_impl;
    }
    constexpr static auto begin(std::false_type/* has_transaction */) -> decltype(sqlite3_module{}.xBegin)
    {
        return nullptr;
    }
    
    static int sync_impl(sqlite3_vtab *pVTab)
    {
        try
        {
            get_module<table_type>(pVTab).sync();
            return SQLITE_OK;
        }
        BOOST_SQLITE_CATCH_AND_RETURN();
    }

    constexpr static auto sync(std::true_type /* has_transaction */) -> decltype(sqlite3_module{}.xSync)
    {
        return &sync_impl;
    }
    constexpr static auto sync(std::false_type/* has_transaction */) -> decltype(sqlite3_module{}.xSync)
    {
        return nullptr;
    }
    
    static int commit_impl(sqlite3_vtab *pVTab)
    {
        try
        {
            get_module<table_type>(pVTab).commit();
            return SQLITE_OK;
        }
        BOOST_SQLITE_CATCH_AND_RETURN();
    }

    constexpr static auto commit(std::true_type /* has_transaction */) -> decltype(sqlite3_module{}.xCommit)
    {
       return &commit_impl;
    }
    constexpr static auto commit(std::false_type/* has_transaction */) -> decltype(sqlite3_module{}.xCommit)
    {
        return nullptr;
    }

    static int rollback_impl(sqlite3_vtab *pVTab)
    {
        try
        {
            get_module<table_type>(pVTab).rollback();
            return SQLITE_OK;
        }
        BOOST_SQLITE_CATCH_AND_RETURN();
    }

    constexpr static auto rollback(std::true_type /* has_transaction */) -> decltype(sqlite3_module{}.xRollback)
    {
        return &rollback_impl;
    }
    constexpr static auto rollback(std::false_type/* has_transaction */) -> decltype(sqlite3_module{}.xRollback)
    {
        return nullptr;
    }

    static int find_function_impl(sqlite3_vtab *pVtab, int nArg, const char *zName,
                                  void (**pxFunc)(sqlite3_context*,int,sqlite3_value**),
                                  void **ppArg)
    {
      try
      {
         return get_module<table_type>(*pVtab).find_function(
              nArg, zName, vtab_function_setter(pxFunc, ppArg));
      }
      catch(...)
      {
          return 0;
      }
    }

    constexpr static auto find_function(std::true_type  = had_find_function{}) -> decltype(sqlite3_module{}.xFindFunction)
    {
       return &find_function_impl;
    }
    constexpr static auto find_function(std::false_type = had_find_function{}) -> decltype(sqlite3_module{}.xFindFunction)
    {
       return nullptr;
    }

    static int rename_impl(sqlite3_vtab *pVTab, const char *zNew)
    {
        try
        {
            get_module<table_type>(*pVTab).rename(zNew);
        }
        BOOST_SQLITE_CATCH_AND_RETURN();
    }

    constexpr static auto rename(rank<0> r) -> int(*)(sqlite3_vtab *, const char*) { return nullptr ;}
    template<typename T = Impl, void(* res)(const char*) = &T::rename>
    constexpr static auto rename(rank<1> r) -> int(*)(sqlite3_vtab *, const char*) { return &rename_impl; }

    constexpr static auto shadow_name(rank<0> r) -> int (*)(const char*) { return nullptr ;}
    template<typename T = Impl, int(* res)(const char*) = &T::shadow_name>
    constexpr static auto shadow_name(rank<1> r) -> int(*)(const char*) { return res ;}


    static int savepoint_impl(sqlite3_vtab *pVTab, int idx)
    {
        try
        {
            get_module<table_type>(*pVTab).savepoint(idx);
        }
        BOOST_SQLITE_CATCH_AND_RETURN();
    }

    constexpr static auto savepoint(std::true_type /* has_recursive_transactions */) -> decltype(sqlite3_module{}.xSavepoint)
    {
      return &savepoint_impl;
    }
    constexpr static auto savepoint(std::false_type/* has_recursive_transactions */) -> decltype(sqlite3_module{}.xSavepoint)
    {
      return nullptr;
    }

    static int release_impl(sqlite3_vtab *pVTab, int idx)
    {
        try
        {
            get_module<table_type>(*pVTab).release(idx);
        }
        BOOST_SQLITE_CATCH_AND_RETURN();
    }

    constexpr static auto release(std::true_type /* has_recursive_transactions */) -> decltype(sqlite3_module{}.xRelease)
    {
      return &release_impl;
    }
    constexpr static auto release(std::false_type/* has_recursive_transactions */) -> decltype(sqlite3_module{}.xRelease)
    {
      return nullptr;
    }

    static int rollback_to_impl(sqlite3_vtab *pVTab, int idx)
    {
        try
        {
            get_module<table_type>(*pVTab).rollback_to(idx);
        }
        BOOST_SQLITE_CATCH_AND_RETURN();
    }

    constexpr static auto rollback_to(std::true_type /* has_recursive_transactions */) -> decltype(sqlite3_module{}.xRollbackTo)
    {
      return &release_impl;
    }
    constexpr static auto rollback_to(std::false_type/* has_recursive_transactions */) -> decltype(sqlite3_module{}.xRollbackTo)
    {
      return nullptr;
    }

    static sqlite3_module make()
    {
        sqlite3_module res{
            3,
            create(eponymous{}, eponymous_only{}),
            &connect_impl,
            best_index(has_best_index{}),
            &disconnect_impl,
            destroy(eponymous{}, eponymous_only{}),
            &open_impl,
            &close_impl,
            filter(has_best_index{}),
            &next_impl,
            &eof_impl,
            &column_impl,
            &row_id_impl,
            update(read_only{}),
            begin( has_transactions{}),
            sync(  has_transactions{}),
            commit(has_transactions{}),
            rollback(has_transactions{}),
            find_function(had_find_function{}),
            rename(rank<1>{})
#if SQLITE_VERSION_NUMBER >= 3007007
          , savepoint(has_recursive_transactions{})
          , release(has_recursive_transactions{})
          , rollback_to(has_recursive_transactions{})
#endif
#if SQLITE_VERSION_NUMBER >= 3026000
          , shadow_name(rank<1>{})
#endif
        };

        return res;
    }
};

template<typename Table>
Table & get_vtable_impl(detail::vtab::cursor_type<Table> * const cursor,
                        std::true_type /* sqlite3_vtab_cursor */)
{
  return get_module<Table>(cursor->pvTab);
}


template<typename Table>
Table & get_vtable_impl(detail::vtab::cursor_type<Table> * const cursor,
                        std::false_type /* sqlite3_vtab_cursor */)
{
  using impl_type = module_impl_helper<sqlite3_vtab_cursor, detail::vtab::cursor_type<Table>>;
  return get_module<Table>(static_cast<impl_type*>(cursor)->pVtab);
}
}
}

////@{
/** @brief Register a vtable
 @ingroup reference
 @see Sqlite vtab reference [vtab](https://www.sqlite.org/vtab.html)

 @param conn   The connection to install the vtable into
 @param name   The name for the vtable
 @param module The module to install as a vtable. See @ref vtab_module_prototype for the structure required
 @param ec     The system::error_code used to deliver errors for the exception less overload.
 @param info   The error_info used to deliver errors for the exception less overload.

 @param The requirements for `module`.

 @returns A reference to the module as stored in the database.


*/
template<typename T>
auto create_module(connection & conn,
                   const char * name,
                   T && module,
                   system::error_code & ec,
                   error_info & ei) -> typename std::decay<T>::type &
{
    using module_type = typename std::decay<T>::type;
    static sqlite3_module mod = detail::vtab::vtable_helper<module_type>::make();

    std::unique_ptr<module_type> p{new module_type(std::forward<T>(module))};
    auto  pp = p.get();

    int res = sqlite3_create_module_v2(
                             conn.handle(), name, &mod, p.release(),
                             +[](void * ptr){delete static_cast<module_type*>(ptr);});

    if (res != SQLITE_OK)
    {
       BOOST_SQLITE_ASSIGN_EC(ec, res);
       ei.set_message(sqlite3_errmsg(conn.handle()));
    }
    return *pp;
}

template<typename T>
auto create_module(connection & conn,
                   const char * name,
                   T && module)  -> typename std::decay<T>::type &
{
    system::error_code ec;
    error_info ei;
    T & ref = create_module(conn, name, std::forward<T>(module), ec, ei);
    if (ec)
        detail::throw_error_code(ec, ei);
    return ref;
}
///@}

/// Returns the table associated with the cursor
/// @ingroup reference
template<typename Table>
Table & get_vtable(detail::vtab::cursor_type<Table> * const cursor)
{
  return detail::vtab::get_vtable_impl<Table>(
        cursor,
        std::is_base_of<sqlite3_vtab_cursor, detail::vtab::cursor_type<Table>>{});
}

#if defined(BOOST_SQLITE_GENERATING_DOCS)
/** @brief The prototype used by @ref create_module
 @ingroup reference

 Note that the class names are for exposition only - the types are deduced by the library.

 Please see the sqlite reference for the virtual table documentation [vtab](https://www.sqlite.org/vtab.html).

 Error handling is implemented through exceptions.
 Members are required unless explicitly stated optional.
The destruction functions are automatically filled with destructors.

*/
struct vtab_module_prototype
{
 /// @brief Can be used to make the table
 constexpr static bool eponymous_only = false;

 /// @brief Creates the instance
 /// The instance_type gets used & managed by value, OR a pointer to a class that inherits sqlite3_vtab.
 /// Optional member - can be skipped for eponymous tables
 /// instance_type must have a member `declaration` that returns a `const char *` for the declaration.
 table_type create(int argc, const char * const argv);

 /// @brief Create a table
 /// The table_type gets used & managed by value, OR a pointer to a class that inherits sqlite3_vtab.
 /// table_type must have a member `declaration` that returns a `const char *` for the declaration.
 table_type connect(int argc, const char * const argv);

 /// The type produced by connect. The type is purely for exposition.
 struct table_type
 {
   /// The Table declaration to be used with  sqlite3_declare_vtab
   const char *declaration();

   /// Destroy the storage = this function needs to be present for non epotymous tables
   void destroy();

   /// Tell sqlite how to communicate with the table.
   /// Optional, this library will fill in a default function that leaves comparisons to sqlite.
   void best_index(sqlite3_index_info* info);

   /// @brief Start a search on the table.
   /// The cursor_type gets used & managed by value, OR a pointer to a class that inherits sqlite3_vtab_cursor.
   cursor_type open();

   /// Cursor needs the following member
   struct cursor_type
   {
      /// @brief Apply a filter to the cursor. Required when best_index is implemented.
      void filter(int id_num, const char * id_str, boost::span<value> values);

      /// @brief Returns the next row.
      void next();

      /// @brief Check if the cursor is and the end
      void eof();

      /// @brief Returns the result of a value. It will use the set_result functionality to create a an sqlite function.
      /// see [vtab_no_change](https://www.sqlite.org/c3ref/vtab_nochange.html)
      T column(int idx, bool no_change):

      /// @brief Returns the id of the current row
      sqlite3_int64 row_id();
   };

   ///@{
   /// Group of functions for modifications. Optional, can be omitted for read only tables.
   /// The return value is the row_id
   /// @brief Delete row
   void delete_(sqlite::value key);
   /// @brief Inert a new row
   sqlite_int64 insert(sqlite::value key, span<sqlite::value> values);
   /// @brief Update the row
   sqlite_int64 update(sqlite::value old_key, sqlite::value new_key, span<sqlite::value> values);
   ///@}

   ///@{
   /// Group of functions to support transactions. Optional.
   /// @brief Begin a tranasction
   void begin();
   /// @brief synchronize the state
   void sync();
   /// @brief commit the transaction
   void commit();
   //// @brief rollback the transaction
   void rollback();
   ///@}


   /// Function to rename the table. Optional
   void rename(const char * new_name);

   ///@{
   /// Support for recursive transactions
   /// @brief Save the current state with to `i`
   void savepoint(int i);
   /// @brief Release all saves states down to `i`
   void release(int i);
   /// @brief Roll the transaction back to `i`.
   void rollback_to(int i);
   ///@}

   /// Optional static member, in case the implementation uses a shadow-function
   static int shadow_name(const char * name);
 };
};
#endif

BOOST_SQLITE_END_NAMESPACE

#endif //BOOST_SQLITE_VTABLE_HPP
