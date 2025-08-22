# boost_sqlite

This library provides a simple C++ sqlite wrapper.

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

If you want to use it for extensions you'll need to 
define `BOOST_SQLITE_COMPILE_EXTENSION` or include `boost/sqlite/extensions.hpp` first.

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

Now that we have the values in the table, let's add a custom aggregate function to create a comma separated list:

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
for (boost::sqlite::row r : conn.query(
    "select first_name, collect_libs(name) "
       " from author inner join library l on author.id = l.author group by last_name"))
  std::cout << r.at(0u).get_text() << " authored " << r.at(1u).get_text() << std::endl;
```

Alternatively a query result can also be read manually instead of using a loop:

```cpp
boost::sqlite::row r;
boost::sqlite::query q = conn.query(
    "select first_name, collect_libs(name) " 
       " from author inner join library l on author.id = l.author group by last_name")
do 
{
  auto r = q.current();''
  std::cout << r.at(0u).get_text() << " authored " << r.at(1u).get_text() << std::endl;
}
while (q.read_next());

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

## Typed queries

Queries can be typed through tuples, describe or, if you're on C++20, by plain structs.
The type to hold them is `static_resultset<T>` which will check if the columns match the result types before usage.
Tuples are matched by position, structs by name.

```cpp
for (auto q : conn.query<std::tuple<std::string, std::string>>(
    "select first_name, collect_libs(name) "
       " from author inner join library l on author.id = l.author group by last_name"))
  std::cout << std::get<0>(q) << " authored " << std::get<0>(q) << std::endl;
```


```cpp
struct query_result { std::string first_name, lib_name;};
BOOST_DESCRIBE_STRUCT(query_result, (), (first_name, lib_name)); // this can be omitted with C++20. 

for (auto q : conn.query<query_result>(
    "select first_name, collect_libs(name) as lib_name"
       " from author inner join library l on author.id = l.author group by last_name"))
  std::cout << q.first_name << " authored " << q.lib_name << std::endl;
```

The following types are allowed in a static query result:

 - `sqlite::value`
 - `int`
 - `sqlite_int64`
 - `double`
 - `std::string`
 - `sqlite::string_view`
 - `sqlite::blob`
 - `sqlite::blob_view`


You'll need to include `boost/sqlite/static_resultset.hpp` for this to work.

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

select assert((3 * 4) = 12);
```

To build a plugin you need to define `BOOST_SQLITE_COMPILE_EXTENSION` 
(e.g. by including `boost/sqlite/extension.hpp` or linking against `boost_sqlite_ext`).

This will include the matching sqlite header (`sqlite3ext.h`) and 
will move all the symbols into an inline namespace `ext` inside `boost::sqlite`.  

<a name="api-reference"></a>
## Reference

* [Reference](#reference): Covers the topics discussed in this document.

## Library Comparisons

While there are many sqlite wrappers out there, most haven't been updated in the last five years - while sqlite has.

Here are some actively maintained ones:

 - [SQLiteCpp](https://github.com/SRombauts/SQLiteCpp) 

SQLiteCpp is the closest to this library, a C++11 wrapper only depending on sqlite & the STL.
It's great and served as an inspiration for this library. 
boost.sqlite does provide more functionality when it comes to hooks, custom functions & virtual tables. 
Furthermore, boost.sqlite has a non-throwing interface and supports variants & json, as those are available through boost.

 - [sqlite_modern_cpp](https://github.com/SqliteModernCpp/sqlite_modern_cpp)

This library takes a different approach, by making everything an `iostream` interface.
`iostream` interfaces have somewhat fallen out of favor. 

 - [sqlite_orm](https://github.com/fnc12/sqlite_orm)

As the name says, it's an ORM. While there is nothing wrong with ORMs, they are one layer of abstraction
above a client library like this.

 - [SOCI](https://github.com/SOCI/soci)

SOCI is an abstraction layer for multiple databases in C++, including sqlite. 
It's interfaces encourages dynamic building of query string, which should not be considered safe.

