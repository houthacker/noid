/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_CONCURRENT_CONCEPTS_H_
#define NOID_SRC_BACKEND_CONCURRENT_CONCEPTS_H_

#include <concepts>
#include <type_traits>

namespace noid::backend::concurrent {

template<typename T>
concept BasicLockable = requires(T basic_lockable) {
  { basic_lockable.lock() } -> std::same_as<std::void_t<T>>;
  { basic_lockable.unlock() } -> std::same_as<std::void_t<T>>;
};

template<typename T>
concept Lockable = BasicLockable<T> and requires(T lockable) {
  { lockable.try_lock() } -> std::convertible_to<bool>;
};

template<typename T>
concept SharedLockable = requires(T shared_lockable) {
  { shared_lockable.lock_shared() } -> std::same_as<std::void_t<T>>;
  { shared_lockable.try_lock_shared() } -> std::convertible_to<bool>;
  { shared_lockable.unlock_shared() } -> std::same_as<std::void_t<T>>;
};

template<typename T>
concept Mutex = Lockable<T>
    and std::is_default_constructible_v<T>
        and std::is_destructible_v<T>
            and (not std::copyable<T>)
            and (not std::movable<T>);

template<typename T>
concept SharedMutex = Mutex<T>
    and SharedLockable<T>;
}


// The compiler doesn't complain about unused instances of std::unique_lock, so let it also not complain
// about our Lockable concept.
#ifndef NoidLock
#define NoidLock [[maybe_unused]] Lockable
#endif

// The compiler doesn't complain about unused instances of std::shared_lock, so let it also not complain
// about our SharedLockable concept.
#ifndef NoidSharedLock
#define NoidSharedLock [[maybe_unused]] SharedLockable
#endif

#endif //NOID_SRC_BACKEND_CONCURRENT_CONCEPTS_H_
