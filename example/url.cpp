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
#include <boost/url/src.hpp>

using namespace boost;


constexpr int pct_subtype = static_cast<int>('U');
constexpr int segment_subtype = static_cast<int>('S');

void tag_invoke(sqlite::set_result_tag, sqlite3_context * ctx, urls::pct_string_view value)
{
  using boost::sqlite::ext::sqlite3_api;
  sqlite3_result_text(ctx, value.data(), value.size(), nullptr);
  sqlite3_result_subtype(ctx, pct_subtype);
}


void tag_invoke(sqlite::set_result_tag, sqlite3_context * ctx, const urls::segments_encoded_view & value)
{
  using boost::sqlite::ext::sqlite3_api;
  sqlite3_result_text(ctx, value.buffer().data(), value.buffer().size(), nullptr);
  sqlite3_result_subtype(ctx, pct_subtype);
}

struct url_table
{
  constexpr static bool eponymous_only = false;

  struct url_wrapper
  {
    urls::url value;
    constexpr static const char * declaration()
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

    struct cursor
    {
      urls::url_view view;
      bool done{false};

      void next() { done = true; }

      sqlite3_int64 row_id() {return static_cast<sqlite3_int64>(0);}
      variant2::variant<variant2::monostate, urls::string_view, urls::pct_string_view> column(int i, bool & nochange)
      {
        nochange = true;
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
      void filter(int idx, const char * idxStr, span<sqlite::value> values)
      {
        if (values.size() > 0u)
          view = urls::parse_uri(values[0].get_text()).value();
      }

      bool eof() const noexcept {return done || view.empty();}
    };

    cursor open()
    {
      return cursor{value};
    }

    void best_index(sqlite3_index_info* info)
    {

      for (auto i = 0; i < info->nConstraint; i ++)
      {
        if (info->aConstraint[i].iColumn == 8
            && info->aConstraint[i].usable)
        {
          if (info->aConstraint[i].op != SQLITE_INDEX_CONSTRAINT_EQ)
            throw std::logic_error("query only support equality constraints");

          info->aConstraintUsage[i].argvIndex = 1;
          info->idxNum = 1;
        }
      }
    }
  };

  url_wrapper connect(int argc, const char * const *argv)
  {
      return url_wrapper{};
  }
};


struct segments_table
{
  constexpr static bool eponymous_only = false;

  struct segment_wrapper
  {
    urls::segments_encoded_view value;
    constexpr static const char * declaration()
    {
      return R"(
          create table segments(
              idx integer,
              segment text,
              segments text hidden);)";
    }

    struct cursor
    {
      urls::segments_encoded_view view;
      urls::segments_encoded_view::const_iterator itr{view.begin()};

      void next() { itr++; }

      sqlite3_int64 row_id() {return std::distance(view.begin(), itr);}
      variant2::variant<variant2::monostate, int, urls::string_view, urls::segments_encoded_view> column(int i, bool & nochange)
      {
        nochange = true;
        switch (i)
        {
          case 0: return std::distance(view.begin(), itr);
          case 1: return *itr;
          case 2: return view;
          default:
            return variant2::monostate{};
        }
      }
      void filter(int idx, const char * idxStr, span<sqlite::value> values)
      {
        if (values.size() > 0u)
          view = urls::segments_encoded_view(values[0].get_text());
        itr = view.begin();
      }
      bool eof() const noexcept {return itr == view.end();}
    };

    cursor open()
    {
      return cursor{value};
    }

    void best_index(sqlite3_index_info* info)
    {
      for (auto i = 0; i < info->nConstraint; i ++)
      {
        if (info->aConstraint[i].iColumn == 2
         && info->aConstraint[i].usable)
        {
          if (info->aConstraint[i].op != SQLITE_INDEX_CONSTRAINT_EQ)
            throw std::logic_error("segments only support equality constraints");
          info->aConstraintUsage[i].argvIndex = 1;
          info->idxNum = 1;
        }
      }

    }
  };

  segment_wrapper connect(int argc, const char * const *argv)
  {
    return segment_wrapper{};
  }
};


struct query_table
{
  constexpr static bool eponymous_only = false;

  struct query_wrapper
  {
    constexpr static const char * declaration()
    {
      return R"(
          create table queries(
              idx integer,
              name text,
              value text,
              query_string text hidden);)";
    }

    struct cursor
    {
      urls::params_encoded_view view;
      urls::params_encoded_view::const_iterator itr{view.begin()};

      void next() { itr++; }

      sqlite3_int64 row_id() {return std::distance(view.begin(), itr);}
      variant2::variant<variant2::monostate, int, urls::string_view, urls::pct_string_view> column(int i, bool & nochange)
      {
        nochange = true;
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
      void filter(int idx, const char * idxStr, span<sqlite::value> values)
      {
        if (values.size() > 0u)
          view = urls::params_encoded_view(values[0].get_text());
        itr = view.begin();
      }
      bool eof() const noexcept {return itr == view.end();}
    };

    cursor open()
    {
      return cursor{};
    }

    void best_index(sqlite3_index_info* info)
    {
      for (auto i = 0; i < info->nConstraint; i ++)
      {
        if (info->aConstraint[i].iColumn == 3
         && info->aConstraint[i].usable)
        {
          if (info->aConstraint[i].op != SQLITE_INDEX_CONSTRAINT_EQ)
            throw std::logic_error("query only support equality constraints");

          info->aConstraintUsage[i].argvIndex = 1;
          info->idxNum = 1;
        }
      }
    }
  };

  query_wrapper connect(int argc, const char * const *argv)
  {
    return query_wrapper{};
  }
};

BOOST_SQLITE_EXTENSION(url, conn)
{
  sqlite::create_module(conn, "url", url_table());
  sqlite::create_module(conn, "segments", segments_table());
  sqlite::create_module(conn, "query", query_table());
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