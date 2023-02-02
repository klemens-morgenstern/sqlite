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

    BOOST_SQLITE_DECL bool read_one(row& r, error_code & ec, error_info & ei);
    BOOST_SQLITE_DECL bool read_one(row & r);

    std::size_t column_count() const
    {
      return sqlite3_column_count(impl_.get());
    }

    core::string_view column_name(std::size_t idx) const
    {
      return sqlite3_column_name(impl_.get(), idx);
    }

    core::string_view table_name(std::size_t idx) const
    {
      return sqlite3_column_table_name(impl_.get(), idx);
    }

    core::string_view column_origin_name(std::size_t idx) const
    {
      return sqlite3_column_origin_name(impl_.get(), idx);
    }
    struct iterator
    {
      using value_type = row;
      using difference_type   = int;
      using reference         = field&;
      using iterator_category = std::random_access_iterator_tag;

      iterator() {}
      explicit iterator(sqlite3_stmt * stmt, bool sentinel) : sentinel_(sentinel )
      {
          row_.stm_ = stmt;
          if (!sentinel && sqlite3_data_count(row_.stm_) == 0)
              (*this)++;

      }

      bool operator!=(iterator rhs)
      {
        return sentinel_ != rhs.sentinel_;
      }

      row &operator*()  { return row_; }
      row *operator->() { return &row_; }

      iterator operator++()
      {
        if (sentinel_)
          return *this;

        auto cc = sqlite3_step(row_.stm_);
        if (cc == SQLITE_DONE)
          sentinel_ = true;
        else if (cc != SQLITE_ROW)
        {
          system::error_code ec;
          BOOST_SQLITE_ASSIGN_EC(ec, cc);
          throw_exception(system_error(ec, sqlite3_errmsg(sqlite3_db_handle(row_.stm_))));
        }
        return *this;
      }

      void operator++(int)
      {
        ++(*this);
      }

     private:
      row row_;
      bool sentinel_ = true;
    };

    iterator begin() { return iterator(impl_.get(), done_);}
    iterator   end() { return iterator(impl_.get(), true); }

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
