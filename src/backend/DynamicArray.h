/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_DYNAMICARRAY_H
#define NOID_DYNAMICARRAY_H

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

namespace noid::backend {

/**
 * @brief Dynamic array class. Implementation copied from a c++14 proposal.
 * @see https://www.open-std.org/JTC1/SC22/WG21/docs/papers/2013/n3532.html
 * @tparam T The array element type.
 */
template<typename T> requires std::equality_comparable<T>
class DynamicArray {
 public:
    // @formatter:off
    typedef       T                               value_type;
    typedef       T&                              reference;
    typedef const T&                              const_reference;
    typedef       T*                              iterator;
    typedef const T*                              const_iterator;
    typedef std::reverse_iterator<iterator>       reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    typedef std::size_t                           size_type;
    typedef ptrdiff_t                             difference_type;
    // @formatter:on

 private:

    T* store;
    size_type count;

    // Helper functions
    void check(size_type n) const
    {
      if (n >= count) {
        throw std::out_of_range("DynamicArray");
      }
    }

    T* alloc_zero(size_type n)
    {
      if (n > std::numeric_limits<size_type>::max() / sizeof(T)) {
        throw std::bad_array_new_length();
      }

      return reinterpret_cast<T*>(new char[n * sizeof(T)](/* Initialize to zero */));
    }

    void dealloc() noexcept {
      if (this->count) {
        for (size_type i = 0; i < count; ++i) {
          (store + i)->~T();
        }
        delete[] store;
      }
    }

 public:

    DynamicArray() = delete;
    DynamicArray& operator=(const DynamicArray&) = delete;

    explicit DynamicArray(size_type c)
        :store(alloc_zero(c)), count(c)
    {
      size_type i = 0;
      try {
        for (; i < count; ++i) {
          new(store + i) T;
        }
      }
      catch (...) {
        for (; i > 0; --i) {
          (store + (i - 1))->~T();
        }
        throw;
      }
    }

    DynamicArray(const DynamicArray& other)
        :store(alloc_zero(other.count)), count(other.count)
    {
      try {
        std::uninitialized_copy(other.begin(), other.end(), this->begin());
      }
      catch (...) {
        delete store;
        throw;
      }
    }

    DynamicArray(DynamicArray&& other) noexcept : store(other.store), count(other.count) {
      other.count = 0;
    }

    DynamicArray& operator=(DynamicArray&& other) noexcept {
      if (this == &other) {
        return *this;
      }

      // Clean up any previous allocated memory
      this->dealloc();

      // Re-initialize this instance
      this->store = other.store;
      this->count = other.count;

      // And leave the other instance in some valid state
      other.count = 0;

      return *this;
    }

    DynamicArray(std::initializer_list<T> values) :store(alloc_zero(values.size())), count(values.size())
    {
      std::copy(values.begin(), values.end(), this->begin());
    }

    ~DynamicArray() noexcept
    {
      this->dealloc();
    }

    // Equality operators
    bool operator==(const DynamicArray<T>& other) const {
      if (this->count != other.count) {
        return false;
      }

      for (size_type i = 0; i < this->count; i++) {
        if (this->store[i] != other[i]) {
          return false;
        }
      }

      return true;
    }

    bool operator!=(const DynamicArray<T>& other) const {
      return !(this == other);
    }

    // Iterators
    // @formatter:off
    iterator        begin()         { return store; }
    const_iterator  begin()   const { return store; }
    const_iterator  cbegin()  const { return store; }
    iterator        end()           { return store + count; }
    const_iterator  end()     const { return store + count; }
    const_iterator  cend()    const { return store + count; }

    reverse_iterator        rbegin()        { return reverse_iterator(end()); }
    const_reverse_iterator  rbegin()  const { return reverse_iterator(end()); }
    reverse_iterator        rend()          { return reverse_iterator(begin()); }
    const_reverse_iterator  rend()    const { return reverse_iterator(begin()); }

    // Capacity
    [[nodiscard]] size_type size()      const { return count; }
    [[nodiscard]] size_type max_size()  const { return count; }
    [[nodiscard]] bool empty()          const { return count == 0; }

    // Element access
    reference       operator[](size_type n)       { return store[n]; }
    const_reference operator[](size_type n) const { return store[n]; }

    reference       front()       { return store[0]; }
    const_reference front() const { return store[0]; }
    reference       back()        { return store[count - 1]; }
    const_reference back()  const { return store[count - 1]; }

    const_reference at(size_type n) const { check(n); return store[n]; }
    reference       at(size_type n)       { check(n); return store[n]; }

    // Data access
    T*        data()        { return store; }
    const T*  data()  const { return store; }
    // @formatter:on
};

}

#endif //NOID_DYNAMICARRAY_H
