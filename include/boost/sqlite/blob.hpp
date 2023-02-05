// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_BLOB_HPP
#define BOOST_SQLITE_BLOB_HPP

#include <boost/sqlite/detail/config.hpp>
#include <type_traits>
#include <memory>
#include <cstring>

BOOST_SQLITE_BEGIN_NAMESPACE

/// @brief a view to a binary large object  @ingroup reference
struct blob_view
{
    /// The data in the blob
    const void * data() const {return data_;}
    /// The size of the data int he blob, in bytes
    std::size_t size() const {return size_;}
    /// Construct a blob
    blob_view(const void * data, std::size_t size) : data_(data), size_(size) {}

    /// Construct a blob from some other blob-like structure.
    template<typename T>
    explicit blob_view(const T & value,
                       typename std::enable_if<
                           std::is_pointer<decltype(std::declval<T>().data())>::value
                        && std::is_convertible<decltype(std::declval<T>().size()), std::size_t>::value
                           >::type * = nullptr) : data_(value.data()), size_(value.size()) {}

    inline blob_view(const struct blob & b);
  private:
    const void * data_{nullptr};
    std::size_t size_{0u};
};

/// @brief an object that holds a binary large object  @ingroup reference
struct blob
{
    /// The data in the blob
    void * data() const {return impl_.get();}
    /// The size of the data int he blob, in bytes
    std::size_t size() const {return size_;}

    /// Create a blob from a blob_view
    explicit blob(blob_view bv)
    {
        impl_.reset(operator new(bv.size()));
        size_ = bv.size();
        std::memcpy(impl_.get(), bv.data(), size_);
    }
    /// Create an empty blob with size `n`.
    explicit blob(std::size_t n) : impl_(operator new(n)), size_(n) {}
    /// Release & take ownership of the blob.
    void * release() && {return std::move(impl_).release(); }
   private:
    struct deleter_ {void operator()(void *p) {operator delete(p);}};
    std::unique_ptr<void, deleter_> impl_;
    std::size_t size_{0u};
};


blob_view::blob_view(const blob & b) : data_(b.data()), size_(b.size()) {}

BOOST_SQLITE_END_NAMESPACE


#endif //BOOST_SQLITE_BLOB_HPP
