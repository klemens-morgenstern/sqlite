# boost_sqlite

This library provides a simple C++ sqlite library.

It includes:

 - typed queries
 - prepared statements
 - json support
 - custom functions (scalar, aggregrate, windows)
 - event hooks
 - virtual tables

sqlite provides an excellent C-API, so this library does not attempt to hide, but to augment it.

## Building the library

You can either build the library and link against `boost_sqlite` for embedding it, 
or `boost_sqlite_ext` for extensions.

Alternatively you can include `boost/sqlite/src.hpp`. If you want to use it for extensions
you'll need to define `BOOST_SQLITE_COMPILE_EXTENSION` or include `boost/sqlite/extensions.hpp` first.

## Quickstart

First we open a database. Note that this can be `":memory:"` for an in-memory database.

```cpp
boost::sqlite::connection conn{"./my-database.db"};
```

Next we're creating tables using boost::sqlite::connection::execute, 
because it can execute multiple statements in one command:

```cpp
  conn.execute(R"(
create table if not exists author (
    id         integer primary key autoincrement,
    first_name text,
    last_name  text
);

create table if not exists library(
    id      integer primary key autoincrement,
    name    text unique,
    author  integer references author(id)
);
)"
);
```

Next, we'll use a prepared statement to insert multiple values by index:

```cpp
conn.prepare("insert into author (first_name, last_name) values (?1, ?2), (?3, ?4), (?5, ?6), (?7, ?8)")
    .execute({"vinnie", "falco", "richard", "hodges", "ruben", "perez", "peter", "dimov"});
```

Prepared statements can also be used multiple time and used with named parameters instead of indexed.

```cpp
{
  conn.query("begin transaction;");

  auto st = conn.prepare("insert into library (\"name\", author) values ($library, "
                         "  (select id from author where first_name = $fname and last_name = $lname))");

  st.execute({{"library", "beast"},    {"fname", "vinnie"}, {"lname", "falco"}});
  st.execute({{"library", "mysql"},    {"fname", "ruben"},  {"lname", "perez"}});
  st.execute({{"library", "mp11"},     {"fname", "peter"},  {"lname", "dimov"}});
  st.execute({{"library", "variant2"}, {"fname", "peter"},  {"lname", "dimov"}});

  conn.query("commit;");
}
```

Now that we have the values in the table, let's add a customn aggregate function to create a comma separated list:

```cpp
struct collect_libs
{
  void step(std::string & name, span<sqlite::value, 1> args)
  {
    if (name.empty())
      name = args[0].get_text();
    else
      (name += ", ") += args[0].get_text();
  }
  std::string final(std::string & name) { return name; }
};
sqlite::create_aggregate_function(conn, "collect_libs", collect_libs{}); 
```

Print out the query with aggregates libraries:

```cpp
for (boost::sqlite::query q : conn.query(
    "select first_name, collect_libs(name) "
       " from author inner join library l on author.id = l.author group by last_name"))
  std::cout << q.at(0u).get_text() << " authored " << q.at(1u).get_text() << std::endl;
```

Alternatively a query result can also be read manually instead of using a loop:

```cpp
boost::sqlite::row r;
boost::sqlite::query q = conn.query(
    "select first_name, collect_libs(name) " 
       " from author inner join library l on author.id = l.author group by last_name")
whilte (q.read_one(r))
  std::cout << q.at(0u).get_text() << " authored " << q.at(1u).get_text() << std::endl;

```

## Fields, values & parameters

sqlite3 has a weak typesystem, where everything is one of 
following [value_types](@ref boost::sqlite::value_type):

 - `integer`
 - `floating`
 - `text`
 - `blob`
 - `null`

The result of a query is a [field](@ref boost::sqlite::field) type, 
while a [value](@ref boost::sqlite::value) is used in functions.

Fields & values can have [subtypes](https://www.sqlite.org/c3ref/value_subtype.html), 
while parameter to prepared statements do not have thos associated. 

Because of this the values that can be bound to an [execute](@ref boost::sqlite::statement::execute) 
need to be convertible to a fixed set of types (see [param_ref](@ref boost::sqlite::param_ref) for details).

When a [value](@ref boost::sqlite::value) is returned from a custom function, 
such as done through [create_scalar_function](@ref boost::sqlite::create_scalar_function), additional types
can be added with the following tag_invoke function:

```cpp
void tag_invoke(const struct set_result_tag &, sqlite3_context * ctx, const my_type & value);
```

An implementation can look like this:

```cpp
void tag_invoke(const struct set_result_tag &, sqlite3_context * ctx, const my_type & value)
{
  auto data = value.to_string();
  sqlite3_result_text(ctx, data.c_str(), data.size(), sqlite3_free);
  sqlite3_result_subtype(ctx, my_subtype);
}

```

## Custom functions

Since sqlite is running in the same process you can add custom functions that can be used from within sqlite. 

 - [collation](@ref boost::sqlite::create_collation)
 - [scalar function](@ref boost::sqlite::create_scalar_function)
 - [aggregate function](@ref boost::sqlite::create_aggregate_function)
 - [window function](@ref boost::sqlite::create_window_function)

## Vtables

This library also simplifies adding virtual tables significantly; 
virtual tables are table that are backed by code instead of data.

See [create_module](@ref boost::sqlite::create_module) and [prototype](@ref boost::sqlite::vtab_module_prototype) for more details.

## Modules

This library can also be used to build a sql plugin:

```cpp
BOOST_SQLITE_EXTENSION(testlibrary, conn)
{
  // create a function that can be used in the plugin
  create_scalar_function(
    conn, "assert",
    [](boost::sqlite::context<>, boost::span<boost::sqlite::value, 1u> sp)
    {
        if (sp.front().get_int() == 0)
          throw std::logic_error("assertion failed");
    });
}
```

The plugin can then be loaded & used like this:

```sql
SELECT load_extension('./test_library');

select asset((3 * 4) = 12);
```

To build a plugin you need to define `BOOST_SQLITE_COMPILE_EXTENSION` 
(e.g. by including `boost/sqlite/extension.hpp` or linking against `boost_sqlite_ext`).

This will include the matching sqlite header (`sqlite3ext.h`) and 
will move all the symbols into an inline namespace `ext` inside `boost::sqlite`.  

<a name="api-reference"></a>
## Reference

* [Reference](#reference): Covers the topics discussed in this document.
