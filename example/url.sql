SELECT load_extension('./url');

-- invoke the url function to get any url by it's elements
select scheme, user, password, host , port, path, query, fragment, "url"
    from url('ws://echo.example.com/?name=boost&thingy=foo&name=sqlite&#demo');

-- table-ize the segments of url
select idx, segment from segments('/foo/bar/foo/xyz');

-- tag::query[]
-- table-ize the query of url
select * from query('name=boost&thingy=foo&name=sqlite&foo');
select * from query where query_string = 'name=boost&thingy=foo&name=sqlite&foo';
-- end::query[]

-- do a left join on the table, so we can use the table function to normalize data.
select host , query.name, query.value
from url('ws://echo.example.com/?name=boost&thingy=foo&name=sqlite#demo') left join query on query.query_string = url.query;

