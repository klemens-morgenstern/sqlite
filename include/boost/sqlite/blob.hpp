// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_BLOB_HPP
#define BOOST_SQLITE_BLOB_HPP

#include <type_traits>
#include <memory>

namespace boost {
namespace sqlite {

struct blob_view
{
    const void * data() const {return data_;}
    std::size_t size() const {return size_;}

    blob_view(const void * data, std::size_t size) : data_(data), size_(size) {}
    template<typename T,
             typename = std::enable_if_t<std::is_pointer<decltype(std::declval<T>().data())>::value>,
             typename = std::enable_if_t<std::is_convertible<decltype(std::declval<T>().size()), std::size_t>::value>>
    explicit blob_view(const T & value) : data_(value.data()), size_(value.size()) {}

  private:
    const void * data_{nullptr};
    std::size_t size_{0u};
};

struct blob
{
    void * data() {return impl_.get();}
    std::size_t size() const {return size_;}

    explicit blob(blob_view bv)
    {
        impl_.reset(operator new(bv.size()));
        size_ = bv.size();
        std::memcpy(impl_.get(), bv.data(), size_);
    }

    explicit blob(std::size_t n) : impl_(operator new(n)), size_(n) {}

  private:
    struct deleter_ {void operator()(void *p) {operator delete(p);}};
    std::unique_ptr<void, deleter_> impl_;
    std::size_t size_{0u};
};

}
}

#endif //BOOST_SQLITE_BLOB_HPP
