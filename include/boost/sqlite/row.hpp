// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_ROW_HPP
#define BOOST_SQLITE_ROW_HPP

#include <boost/sqlite/field.hpp>

namespace boost {
namespace sqlite {

struct row
{
    std::size_t size() const
    {
        return sqlite3_column_count(stm_);
    }

    field at(std::size_t idx) const
    {
        if (idx >= size())
            throw std::out_of_range("column out of range");
        else
        {
             field f;
             f.stm_ = stm_;
             f.col_ = static_cast<int>(idx);
            return f;
        }
    }

    field operator[](std::size_t idx) const
    {
        field f;
        f.stm_ = stm_;
        f.col_ = static_cast<int>(idx);
        return f;
    }

    struct const_iterator
    {
        using difference_type   = int;
        using reference         = field&;
        using iterator_category = std::random_access_iterator_tag;

        const_iterator & operator++()
        {
            f_.col_++;
            return *this;
        }

        const_iterator operator++(int)
        {
            auto last = *this;
            ++(*this);
            return last;
        }

        const_iterator & operator--()
        {
            f_.col_--;
            return *this;
        }

        const_iterator operator--(int)
        {
            auto last = *this;
            --(*this);
            return last;
        }

        field operator[](int i) const
        {
            auto f = f_;
            f.col_ += i;
            return f;
        }

        const_iterator operator+(int i) const
        {
            auto r = *this;
            r.f_.col_ += i;
            return r;
        }

        const_iterator operator-(int i) const
        {
            auto r = *this;
            r.f_.col_ -= i;
            return r;
        }

        const_iterator & operator+=(int i)
        {
            f_.col_ += i;
            return *this;
        }

        const_iterator & operator-=(int i)
        {
            f_.col_ -= i;
            return *this;
        }

        const field & operator*() const
        {
            return f_;
        }

        const field * operator->() const
        {
            return &f_;
        }

        bool operator==(const const_iterator& other) const
        {
            return f_.col_ == other.f_.col_
                && f_.stm_ == other.f_.stm_;
        }

        bool operator!=(const const_iterator& other) const
        {
            return f_.col_ != other.f_.col_
                || f_.stm_ != other.f_.stm_;
        }

        bool operator<(const const_iterator& other) const
        {
            return f_.col_ < other.f_.col_
                && f_.stm_ < other.f_.stm_;
        }

        bool operator>(const const_iterator& other) const
        {
            return f_.col_ > other.f_.col_
                && f_.stm_ > other.f_.stm_;
        }

        const_iterator() = default;
      private:
        friend struct row;
        field f_;
    };

    const_iterator begin() const
    {
        const_iterator ci;
        ci.f_.col_ = 0;
        ci.f_.stm_ = stm_;
        return ci;
    }

    const_iterator end() const
    {
        const_iterator ci;
        ci.f_.col_ = sqlite3_column_count(stm_);
        ci.f_.stm_ = stm_;
        return ci;
    }
  private:
    friend class resultset;
    sqlite3_stmt * stm_;

};

}
}

#endif //BOOST_SQLITE_ROW_HPP
