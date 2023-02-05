//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/sqlite/extension.hpp>
#include <boost/sqlite/function.hpp>

BOOST_SQLITE_EXTENSION(extension, conn)
{
    create_scalar_function(
        conn, "assert",
        [](boost::sqlite::context<>, boost::span<boost::sqlite::value, 1u> sp)
        {
            if (sp.front().get_int() == 0)
              throw std::logic_error("test failed");
        });

    create_scalar_function(
        conn, "my_add",
        [](boost::sqlite::context<>, boost::span<boost::sqlite::value, 2u> sp)
        {
          return sp[0].get_int() + sp[1].get_int();
        });
}
