//
// Copyright (c) 2024 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SQLITE_TRANSACTION_HPP
#define BOOST_SQLITE_TRANSACTION_HPP

#include <boost/sqlite/connection.hpp>

BOOST_SQLITE_BEGIN_NAMESPACE

/**
 * @brief A simple transaction guard implementing RAAI for transactions
 * @ingroup reference
 *
 *   @par Example
 *   @code{.cpp}
 *     sqlite::connection conn;
 *     conn.connect("./my-database.db");
 *
 *     sqlite::transaction t{conn};
 *     conn.prepare("insert into log (text) values ($1)").execute(std::make_tuple("booting up"));
 *     t.commit();
 *   @endcode
 */
struct transaction
{
  /// The mode of the transaction
  enum behaviour {deferred, immediate, exclusive};
  /// A tag to use, to adopt an already initiated transaction.
  constexpr static struct adopt_transaction_t {} adopt_transaction{};


  /// Create transaction guard on an existing transaction
  transaction(connection_ref conn, adopt_transaction_t) : conn_(conn), completed_(false)
  {
  }


  /// Create transaction guard and initiate a transaction
  transaction(connection_ref conn) : conn_(conn)
  {
    conn.prepare("BEGIN").step();
    completed_ = false;
  }

  /// Create transaction guard and initiate a transaction with the defined behaviour
  transaction(connection_ref conn, behaviour b) : conn_(conn)
  {
    switch (b)
    {
      case deferred:  conn.prepare("BEGIN DEFERRED").step(); break;
      case immediate: conn.prepare("BEGIN IMMEDIATE").step(); break;
      case exclusive: conn.prepare("BEGIN EXCLUSIVE").step(); break;
    }
    completed_ = false;
  }

  // see https://www.sqlite.org/lang_transaction.html re noexcept
  /// rollback the transaction if not committed.
  ~transaction() noexcept(SQLITE_VERSION_NUMBER >= 3007011)
  {
    if (!completed_)
      conn_.prepare("ROLLBACK").step();
  }

  ///@{
  /// Commit the transaction.
  void commit()
  {
    conn_.prepare("COMMIT").step();
    completed_ = true;
  }

  void commit(system::error_code & ec, error_info & ei)
  {
    conn_.prepare("COMMIT").step(ec, ei);
    completed_ = true;
  }
  ///@}

  ///@{
  /// Rollback the transaction explicitly.
  void rollback()
  {
    conn_.prepare("ROLLBACK").step();
    completed_ = true;
  }

  void rollback(system::error_code & ec, error_info & ei)
  {
    conn_.prepare("ROLLBACK").step(ec, ei);
    completed_ = true;
  }
  ///@}

 private:
  connection_ref conn_;
  bool completed_ = true;
};

/**
 * @brief A simple transaction guard implementing RAAI for savepoints. Savepoints can be used recursively.
 * @ingroup reference
 *
 * @par Example
 * @code{.cpp}
 *   sqlite::connection conn;
 *   conn.connect("./my-database.db");
 *
 *   sqlite::savepoint t{conn, "my-savepoint};
 *   conn.prepare("insert into log (text) values ($1)").execute(std::make_tuple("booting up"));
 *   t.commit();
 * @endcode
*/
struct savepoint
{
  /// A tag to use, to adopt an already initiated transaction.
  constexpr static transaction::adopt_transaction_t adopt_transaction{};

  /// Create savepoint guard on an existing savepoint
  savepoint(connection_ref conn, std::string name, transaction::adopt_transaction_t)
      : conn_(conn), name_(std::move(name))
  {
  }

  /// Create transaction guard and initiate it
  savepoint(connection_ref conn, std::string name) : conn_(conn), name_(std::move(name))
  {
    conn.prepare("SAVEPOINT " + name_).step();
    completed_ = false;
  }


  /// rollback to the savepoint if not committed.
  ~savepoint() noexcept(SQLITE_VERSION_NUMBER >= 3007011)
  {
    if (!completed_)
      conn_.prepare("ROLLBACK TO " + name_).step();
  }

  ///@{
  /// Commit/Release the transaction.
  void commit()
  {
    conn_.prepare("RELEASE " + name_).step();
    completed_ = true;
  }

  void commit(system::error_code & ec, error_info & ei)
  {
    conn_.prepare("RELEASE " + name_).step(ec, ei);
    completed_ = true;
  }

  void release()
  {
    conn_.prepare("RELEASE " + name_).step();
    completed_ = true;
  }

  void release(system::error_code & ec, error_info & ei)
  {
    conn_.prepare("RELEASE " + name_).step( ec, ei);
    completed_ = true;
  }
  ///@}

  ///@{
  /// Rollback the transaction explicitly.
  void rollback()
  {
    conn_.prepare("ROLLBACK TO" + name_);
    completed_ = true;
  }

  void rollback(system::error_code & ec, error_info & ei)
  {
    conn_.prepare("ROLLBACK TO " + name_).step( ec, ei);
    completed_ = true;
  }
  ///@}

  /// The name of the savepoint.
  const std::string & name() const {return name_;}
 private:
  connection_ref conn_;
  std::string name_;
  bool completed_ = true;
};


BOOST_SQLITE_END_NAMESPACE

#endif //BOOST_SQLITE_TRANSACTION_HPP
