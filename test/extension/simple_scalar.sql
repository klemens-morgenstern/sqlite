SELECT load_extension('./boost_test_extension_simple_scalar');

select assert(5 = (select my_add(2, 3)));
select assert(7 = (select my_add(4, 3)));
