/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_FIXEDSIZEVECTOR_H
#define NOID_FIXEDSIZEVECTOR_H

#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <vector>

namespace noid::backend {

template<typename T>
class FixedSizeVector {
 private:
    std::size_t count;

    std::vector<T> vec;

 public:
    typedef T value_type;
    typedef typename std::vector<T>::reference reference;
    typedef typename std::vector<T>::const_reference const_reference;
    typedef typename std::vector<T>::iterator iterator;
    typedef typename std::vector<T>::const_iterator const_iterator;
    typedef typename std::vector<T>::size_type size_type;

    constexpr FixedSizeVector() = delete;

    constexpr explicit FixedSizeVector(size_type size)
        :count(size), vec(size) { }

    constexpr FixedSizeVector(size_type count, T& value)
        :count(count), vec(count, value) { }

    constexpr FixedSizeVector(std::initializer_list<T> init)
        :count(init.size()), vec(init) { }

    constexpr FixedSizeVector(const FixedSizeVector& other)
        :count(other.count), vec(other.vec) { }

    constexpr explicit FixedSizeVector(const std::vector<T>& other) : count(other.size()), vec(other) {}

    constexpr FixedSizeVector(FixedSizeVector&& other) noexcept
        :count(other.count), vec(std::move(other.vec))
    {
      other.count = 0;
    }

    [[nodiscard]] std::size_t size() const
    {
      return this->count;
    }

    constexpr T* data() noexcept
    {
      return this->vec.data();
    }

    constexpr const T* data() const noexcept
    {
      return this->vec.data();
    }

    constexpr const std::vector<T>& backing_vector() const noexcept
    {
      return this->vec;
    }

    constexpr reference at(size_type pos) {
      return this->vec.at(pos);
    }

    constexpr const_reference at(size_type pos) const {
      return this->vec.at(pos);
    }

    constexpr reference operator[](size_type i) {
      return this->vec[i];
    }

    constexpr const_reference operator[](size_type i) const
    {
      return this->vec[i];
    }

    constexpr FixedSizeVector& operator=(const FixedSizeVector& other) = default;
    constexpr FixedSizeVector& operator=(FixedSizeVector&& other) noexcept = default;
    constexpr FixedSizeVector& operator=(std::initializer_list<T> init) noexcept = delete;

    constexpr auto operator<=>(const FixedSizeVector<T>& rhs) const
    {
      return this->vec <=> rhs.vec;
    }

    constexpr bool operator==(const FixedSizeVector<T>& rhs) const
    {
      return this->vec == rhs.vec;
    }

    [[nodiscard]] constexpr iterator begin()
    {
      return this->vec.begin();
    }

    [[nodiscard]] constexpr const_iterator begin() const
    {
      return this->vec.begin();
    }

    [[nodiscard]] constexpr iterator end()
    {
      return this->vec.end();
    }

    [[nodiscard]] constexpr const_iterator end() const
    {
      return this->vec.end();
    }
};

}

#endif //NOID_FIXEDSIZEVECTOR_H
