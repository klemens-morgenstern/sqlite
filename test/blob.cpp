//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


#include <boost/sqlite/blob.hpp>
#include <boost/sqlite/connection.hpp>

#include <random>

#include "test.hpp"

using namespace boost;

TEST_CASE("blob")
{
  sqlite::connection conn{":memory:"};
  // language=sqlite
  conn.execute("create table blobs(id integer primary key autoincrement, bb blob);");

  std::vector<unsigned char> blobby;
  blobby.resize(4096*4096);
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0,255); // distribution in range [1, 6]

  std::generate(blobby.begin(), blobby.end(),
                [&]{return static_cast<unsigned char>(dist(rng));});


  conn.prepare("insert into blobs(bb) values ($1);").execute(std::make_tuple(sqlite::zero_blob(4096 * 4096 )));

  auto bh = open_blob(conn, "main", "blobs", "bb", 1);

  CHECK(bh.size() == 4096 * 4096);


  unsigned char buf[4096];
  std::generate(std::begin(buf), std::end(buf), [&]{return static_cast<unsigned char>(dist(rng));});
  bh.read_at(buf, 4096, 4096);
  CHECK(std::all_of(std::begin(buf), std::end(buf), [](unsigned char c) {return c == 0u;}));

  bh.write_at(blobby.data(), blobby.size(), 0u);
  bh.read_at(buf, 4096, 4096);
  CHECK(std::memcmp(buf, blobby.data() + 4096, 4096) == 0);

  CHECK_THROWS(open_blob(conn, "main", "doesnt-exit", "blobber", 2));

  sqlite::blob_handle bb;
  CHECK_THROWS(bb.read_at(blobby.data(), blobby.size(), 0));
  CHECK_THROWS(bb.write_at(blobby.data(), blobby.size(), 0));

}