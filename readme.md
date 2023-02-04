# boost_sqlite

This library provides a simple C++ wrapper around sqlite.

This includes:

 - typed queries
 - prepared statements
 - json support
 - custom functions (scalar, aggegrate, windows)
 - event hooks


## Tutorial

Open a library.
```cpp
sqlite::connection conn{":memory:"};
```

Create tables using `execute`, which can executor multiple statements:

```cpp
  conn.execute(R"(
create table author (
    id         integer primary key autoincrement,
    first_name text,
    last_name  text
);

create table library(
    id      integer primary key autoincrement,
    name    text unique,
    author  integer references author(id)
);
)");
```

Add a few values with a single prepared statement:

```cpp
conn.prepare("insert into author (first_name, last_name) values ($1, $2), ($3, $4), ($5, $6), ($7, $8)")
    .execute(std::make_tuple("vinnie", "falco", "richard", "hodges", "ruben", "perez", "peter", "dimov"));
```

Add more data with a reusable prepare statement & transaction.

```cpp
{
  conn.query("begin transaction;");

  auto st = conn.prepare(R"(insert into library ("name", author) values ($1,
                           (select id from author where first_name = $2 and last_name = $3)))");

  st.execute(std::make_tuple("beast",   "vinnie", "falco"));
  st.execute(std::make_tuple("mysql",    "ruben", "perez"));
  st.execute(std::make_tuple("mp11",     "peter", "dimov"));
  st.execute(std::make_tuple("variant2", "peter", "dimov"));

  conn.query("commit;");
}
```

Add a custom aggregate function to create a comma separated list:

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
for (auto q : conn.query("select first_name, collect_libs(name) from author inner join library l on author.id = l.author group by last_name"))
  std::cout << q.at(0u).get_text() << " authored " << q.at(1u).get_text() << std::endl;
```


<a name="api-reference"></a>
## Reference

* [Reference](#reference): Covers the topics discussed in this document.
