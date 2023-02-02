// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/sqlite/connection.hpp>
#include "doctest.h"

using namespace boost;

TEST_CASE("connection")
{
    sqlite::connection conn;

    conn.connect(":memory:");
    sqlite::row rr;
    CHECK(conn.changes() == 0u);
// language=sqlite
    conn.query(R"(
    CREATE TABLE people(id integer primary key autoincrement, first_name varchar(20), last_name varchar(20));
)").read_one(rr);

// language=sqlite
    conn.prepare_statement(R"(
    insert into people(first_name, last_name) values('richard', $1), ('ruben', $2);
)").execute(std::make_tuple("hodges", "perez"));

    CHECK(conn.changes() == 2u);
    //< lifetime issue here: the value needs to be valid until the return statement completes!
    // this might to lead some surprises, but IMO manageable. different for results!
    auto res = conn.query("select * from people;");

    sqlite::row r;
    while (res.read_one(r))
    {
        CHECK(res.table_name(0u) ==  "people");
        CHECK(res.column_name(0u) == "id");
        CHECK(res.column_name(1u) == "first_name");
        CHECK(res.column_name(2u) == "last_name");
        for (auto c : r)
        {
            std::string tmp{c.get_text()};
            printf("| %s ", tmp.c_str());
        }
        printf("|\n");

    }

}