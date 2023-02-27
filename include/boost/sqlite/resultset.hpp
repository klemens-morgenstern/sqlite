// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_RESULTSET_HPP
#define BOOST_SQLITE_RESULTSET_HPP

#include <memory>
#include <boost/sqlite/row.hpp>

#include <boost/system/result.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE
struct connection ;
/**
  @brief Representation of a result from a database.
  @ingroup reference

  If is a forward-range with input iterators.

  @par Example

  @code{.cpp}

  extern sqlite::connection conn;

  sqlite::resultset rs = conn.query("select * from users;");

  do
  {
    handle_row(r.current());
  }
  while (rs.read_next()) // read it line by line


    @endcode

*/
struct resultset
{
    /// Get the current row.
    row current() const &
    {
        row r;
        r.stm_ = impl_.get();
        return r;
    }
    /// Checks if the last row has been reached.
    bool done() const {return done_;}

    ///@{
    /// Read the next row. Returns false if there's nothing more to read.
    BOOST_SQLITE_DECL bool read_next(error_code & ec, error_info & ei);
    BOOST_SQLITE_DECL bool read_next();
    ///@}

    ///
    std::size_t column_count() const
    {
      return sqlite3_column_count(impl_.get());
    }
    /// Get the name of the column idx.
    core::string_view column_name(std::size_t idx) const
    {
      return sqlite3_column_name(impl_.get(), idx);
    }
    /// Get the name of the source table for column idx.
    core::string_view table_name(std::size_t idx) const
    {
      return sqlite3_column_table_name(impl_.get(), idx);
    }
    /// Get the origin name of the column for column idx.
    core::string_view column_origin_name(std::size_t idx) const
    {
      return sqlite3_column_origin_name(impl_.get(), idx);
    }

    /// The input iterator can be used to read every row in a for-loop
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

    /// Return an input iterator to the currently unread row
    iterator begin() { return iterator(impl_.get(), done_);}
    /// Sentinel iterator.
    iterator   end() { return iterator(impl_.get(), true); }

  private:
    friend struct connection;
    friend struct statement;
    struct deleter_
    {
        constexpr deleter_() noexcept {}
        bool delete_ = true;
        void operator()(sqlite3_stmt * sm)
        {
            if (sqlite3_data_count(sm) > 0)
              while ( sqlite3_step(sm) == SQLITE_ROW);
            if (delete_)
                sqlite3_finalize(sm);
            else
            {
                sqlite3_clear_bindings(sm);
                sqlite3_reset(sm);
            }

        }
    };
    std::unique_ptr<sqlite3_stmt, deleter_> impl_;
    bool done_ = false;
};

BOOST_SQLITE_END_NAMESPACE

#if defined(BOOST_SQLITE_HEADER_ONLY)
#include <boost/sqlite/impl/resultset.ipp>
#endif

#endif //BOOST_SQLITE_RESULTSET_HPP
