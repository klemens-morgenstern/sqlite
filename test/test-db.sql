--query_helper(R"(

create table author (
    id integer primary key autoincrement not null,
    first_name text unique not null,
    last_name text
);

insert into author (first_name, last_name) values
                   ('vinnie', 'falco'),
                   ('richard', 'hodges'),
                   ('ruben', 'perez'),
                   ('peter', 'dimov')
;

create table library(
    id integer  primary key autoincrement,
    name text unique,
    author  integer references author(id)
);

insert into library ("name", author) values
                    ('beast',    (select id from author where first_name = 'vinnie' and last_name = 'falco')),
                    ('mysql',    (select id from author where first_name = 'ruben'  and last_name = 'perez')),
                    ('mp11',     (select id from author where first_name = 'peter'  and last_name = 'dimov')),
                    ('variant2', (select id from author where first_name = 'peter'  and last_name = 'dimov'))
;

--)")