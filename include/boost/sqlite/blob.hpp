// Copyright (c) 2022 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SQLITE_BLOB_HPP
#define BOOST_SQLITE_BLOB_HPP

#include <boost/sqlite/detail/config.hpp>
#include <boost/sqlite/error.hpp>
#include <type_traits>
#include <memory>
#include <cstring>

BOOST_SQLITE_BEGIN_NAMESPACE

struct connection ;

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

/// Helper type to pass a blob full of zeroes without allocating extra memory.
enum class zero_blob : sqlite3_uint64 {};

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

/// @brief an object that holds a binary large object  @ingroup reference
struct blob_handle
{
    /// Default constructor
    blob_handle() = default;

    /// Construct from a handle. Takes owner ship
    explicit blob_handle(sqlite3_blob * blob) : blob_(blob) {}

    ///@{
    /// Reopen on another row
    void reopon(sqlite3_int64 row_id, error_code & ec)
    {
        int res = sqlite3_blob_reopen(blob_.get(), row_id);
        BOOST_SQLITE_ASSIGN_EC(ec, res);
    }
    void reopon(sqlite3_int64 row_id)
    {
        boost::system::error_code ec;
        reopon(row_id, ec);
        if (ec)
            boost::throw_exception(system_error(ec));
    }
    ///@}

    ///@{
    /// Read data from the blob
    void read_at(void *data, int len, int offset, error_code &ec)
    {
        int res = sqlite3_blob_read(blob_.get(), data, len, offset);
        BOOST_SQLITE_ASSIGN_EC(ec, res);
    }
    void read_at(void *data, int len, int offset)
    {
        boost::system::error_code ec;
        read_at(data, len, offset, ec);
        if (ec)
            boost::throw_exception(system_error(ec));
    }
    ///@}

    ///@{
    /// Write data to the blob
    void write_at(const void *data, int len, int offset, error_code &ec)
    {
        int res = sqlite3_blob_write(blob_.get(), data, len, offset);
        BOOST_SQLITE_ASSIGN_EC(ec, res);
    }
    void write_at(const void *data, int len, int offset)
    {
        boost::system::error_code ec;
        write_at(data, len, offset, ec);
        if (ec)
            boost::throw_exception(system_error(ec));
    }
    ///@}

    /// The size of the blob
    std::size_t size() const {return static_cast<std::size_t>(sqlite3_blob_bytes(blob_.get()));}

    /// The handle of the blob
    using handle_type = sqlite3_blob*;
    /// Get the handle of the blob
    handle_type handle() { return blob_.get(); }
    /// Release the owned handle.
    handle_type release() &&    { return blob_.release(); }
 private:
    struct deleter_
    {
        void operator()(sqlite3_blob *impl)
        {
          sqlite3_blob_close(impl);
        }
    };
    std::unique_ptr<sqlite3_blob, deleter_> blob_;
};

///@{
/// Open a blob for incremental access
BOOST_SQLITE_DECL
blob_handle open_blob(connection & conn,
                      const char *db,
                      const char *table,
                      const char *column,
                      sqlite3_int64 row,
                      bool read_only,
                      error_code &ec,
                      error_info &ei);

BOOST_SQLITE_DECL
blob_handle open_blob(connection & conn,
                      const char *db,
                      const char * table,
                      const char *column,
                      sqlite3_int64 row,
                      bool read_only = false);
///}@



BOOST_SQLITE_END_NAMESPACE

#if defined(BOOST_SQLITE_HEADER_ONLY)
#include <boost/sqlite/impl/blob.ipp>
#endif

#endif //BOOST_SQLITE_BLOB_HPP
