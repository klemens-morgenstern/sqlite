//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/sqlite/extension.hpp>
#include <boost/sqlite/vtable.hpp>
#include <boost/system/result.hpp>
#include <boost/url.hpp>

using namespace boost;

// tag::subtype[]
constexpr int pct_subtype = static_cast<int>('U');

void tag_invoke(sqlite::set_result_tag, sqlite3_context * ctx, urls::pct_string_view value)
{
  using boost::sqlite::ext::sqlite3_api;
  // we're using the sqlite API here directly, because we need to set a different subtype
  sqlite3_result_text(ctx, value.data(), value.size(), nullptr);
  sqlite3_result_subtype(ctx, pct_subtype);
}


void tag_invoke(sqlite::set_result_tag, sqlite3_context * ctx, const urls::segments_encoded_view & value)
{
  using boost::sqlite::ext::sqlite3_api;
  // we're using the sqlite API here directly, because we need to set a different subtype
  sqlite3_result_text(ctx, value.buffer().data(), value.buffer().size(), nullptr);
  sqlite3_result_subtype(ctx, pct_subtype);
}
// end::subtype[]

struct url_cursor final
    : sqlite::vtab::cursor<
        variant2::variant<variant2::monostate, core::string_view, urls::pct_string_view>
        >
{
  url_cursor(urls::url_view view) : view(view ) {}

  urls::url_view view;
  bool done{false};

  sqlite::result<void> next() { done = true; return {};}

  sqlite::result<sqlite3_int64> row_id() {return static_cast<sqlite3_int64>(0);}
  sqlite::result<column_type> column(int i, bool /*nochange*/)
  {
    switch (i)
    {
      case 0: return view.scheme();
      case 1: return view.encoded_user();
      case 2: return view.encoded_password();
      case 3: return view.encoded_host();
      case 4: return view.port();
      case 5: return view.encoded_path();
      case 6: return view.encoded_query();
      case 7: return view.encoded_fragment();
      case 8: return view.buffer();
      default:
        return variant2::monostate{};
    }
  }
  sqlite::result<void> filter(int /*idx*/, const char * /*idxStr*/, span<sqlite::value> values)
  {
    if (values.size() > 0u)
      view = urls::parse_uri(values[0].get_text()).value();
    return {};
  }

  bool eof() noexcept {return done || view.empty();}
};

struct url_wrapper final : sqlite::vtab::table<url_cursor>
{
  urls::url value;
  const char * declaration() override
  {
    return R"(
          create table url(
              scheme text,
              user text,
              password text,
              host text,
              port text,
              path text,
              query text,
              fragment text,
              url text hidden);)";
  }

  sqlite::result<url_cursor> open() override
  {
    return url_cursor{value};
  }

  sqlite::result<void> best_index(sqlite::vtab::index_info & info) override
  {
    for (const auto & constraint : info.constraints())
    {
      if (constraint.iColumn == 8 && constraint.usable)
      {
        if (constraint.op != SQLITE_INDEX_CONSTRAINT_EQ)
          return {SQLITE_MISUSE,
                  sqlite::error_info("query only support equality constraints")};
        info.usage_of(constraint).argvIndex = 1;
        info.set_index(1);
      }
    }

    return {};
  }
};


struct url_module final : sqlite::vtab::eponymous_module<url_wrapper>
{
  sqlite::result<url_wrapper> connect(sqlite::connection /*db*/,
                                      int /*argc*/, const char * const */*argv*/)
  {
      return url_wrapper{};
  }
};

struct segements_cursor final : sqlite::vtab::cursor<
    variant2::variant<variant2::monostate, std::int64_t, core::string_view, urls::segments_encoded_view>>
{
  segements_cursor(urls::segments_encoded_view view) : view(view) {}
  urls::segments_encoded_view view;
  urls::segments_encoded_view::const_iterator itr{view.begin()};

  sqlite::result<void> next() override { itr++; return {};}

  sqlite::result<sqlite3_int64> row_id() override {return std::distance(view.begin(), itr);}
  sqlite::result<column_type> column(int i, bool /*nochange*/) override
  {
    //nochange = true;
    switch (i)
    {
      case 0: return std::distance(view.begin(), itr);
      case 1: return *itr;
      case 2: return view;
      default:
        return variant2::monostate{};
    }
  }
  sqlite::result<void> filter(int /*idx*/, const char * /*idxStr*/,
                              span<sqlite::value> values) override
  {
    if (values.size() > 0u)
      view = urls::segments_encoded_view(values[0].get_text());
    itr = view.begin();
    return {};
  }
  bool eof() noexcept override {return itr == view.end();}
};

struct segment_wrapper final : sqlite::vtab::table<segements_cursor>
{
  urls::segments_encoded_view value;
  const char * declaration() override
  {
    return R"(
          create table segments(
              idx integer,
              segment text,
              segments text hidden);)";
  }



  sqlite::result<segements_cursor> open() override
  {
    return segements_cursor{value};
  }

  sqlite::result<void> best_index(sqlite::vtab::index_info & info) override
  {
    for (auto & constraint : info.constraints())
    {
      if (constraint.iColumn == 2
          && constraint.usable)
      {
        if (constraint.op != SQLITE_INDEX_CONSTRAINT_EQ)
          return {SQLITE_OK, sqlite::error_info("segments only support equality constraints")};
        info.usage_of(constraint).argvIndex = 1;
        info.set_index(1);
      }
    }

    return {};
  }
};


struct segments_module final : sqlite::vtab::eponymous_module<segment_wrapper>
{
  sqlite::result<segment_wrapper> connect(sqlite::connection /*conn*/,
                                          int /*argc*/, const char * const */*argv*/)
  {
    return segment_wrapper{};
  }
};

// tag::query_cursor[]
struct query_cursor final : sqlite::vtab::cursor<
    variant2::variant<variant2::monostate, std::int64_t, core::string_view, urls::pct_string_view>
    >
{
  urls::params_encoded_view view;
  urls::params_encoded_view::const_iterator itr{view.begin()};

  sqlite::result<void> next() override { itr++; return {};}

  sqlite::result<sqlite3_int64> row_id() override {return std::distance(view.begin(), itr);}
  sqlite::result<column_type> column(int i, bool /*nochange*/) override // <1>
  {
    //nochange = true;
    switch (i)
    {
      case 0: return std::distance(view.begin(), itr);
      case 1: return itr->key;
      case 2:
        if (!itr->has_value)
          return variant2::monostate{};
        else
          return itr->value;
      case 3: return view.buffer();
      default:
        return variant2::monostate{};
    }
  }
  sqlite::result<void> filter(int /*idx*/, const char * /*idxStr*/,
                              span<sqlite::value> values) override
  {
    if (values.size() > 0u) // <2>
      view = urls::params_encoded_view(values[0].get_text());
    itr = view.begin();

    return {};
  }
  bool eof() noexcept override {return itr == view.end();}
};
// end::query_cursor[]

// tag::query_boiler_plate[]
struct query_wrapper final : sqlite::vtab::table<query_cursor>
{
  const char * declaration() override
  {
    return R"(
          create table queries(
              idx integer,
              name text,
              value text,
              query_string text hidden);)"; // <1>
  }

  sqlite::result<query_cursor> open() override
  {
    return query_cursor{};
  }

  sqlite::result<void> best_index(sqlite::vtab::index_info & info) override
  {
    for (auto & constraint : info.constraints())
    {
      if (constraint.iColumn == 3
          && constraint.usable)
      {
        if (constraint.op != SQLITE_INDEX_CONSTRAINT_EQ) // <2>
          return sqlite::error{SQLITE_OK, "query only support equality constraints"};

        info.usage_of(constraint).argvIndex = 1;
        info.set_index(1);
      }
    }
    return {};
  }
};

struct query_module final : sqlite::vtab::eponymous_module<query_wrapper>
{
  sqlite::result<query_wrapper> connect(sqlite::connection /*conn*/,
                        int /*argc*/, const char * const */*argv*/)
  {
    return query_wrapper{};
  }
};
// end::query_boiler_plate[]

BOOST_SQLITE_EXTENSION(url, conn)
{
  sqlite::create_module(conn, "url", url_module{});
  sqlite::create_module(conn, "segments", segments_module());

  // tag::query_boiler_plate[]
  sqlite::create_module(conn, "query", query_module());
  // end::query_boiler_plate[]
  sqlite::create_scalar_function(
      conn, "pct_decode",
      +[](boost::sqlite::context<> , boost::span<boost::sqlite::value, 1u> s)
      {
          return urls::pct_string_view(s[0].get_text()).decode();
      });

  sqlite::create_scalar_function(
      conn, "pct_encode",
      +[](boost::sqlite::context<> , boost::span<boost::sqlite::value, 1u> s)
      {
        return urls::encode(s[0].get_text(), urls::pchars);
      });
}