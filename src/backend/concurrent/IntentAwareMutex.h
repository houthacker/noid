/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */


#ifndef NOID_SRC_BACKEND_CONCURRENT_INTENTAWAREMUTEX_H_
#define NOID_SRC_BACKEND_CONCURRENT_INTENTAWAREMUTEX_H_

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <shared_mutex>


namespace noid::backend::concurrent {

/**
 * @brief A mutex implementation that denies new shared locks if another thread intends to acquire the unique lock.
 *
 * @details When the @c IntentAwareMutex is used as a @c std::mutex, it acts as if it is a @c std::mutex.
 * When used as a @c std::shared_mutex however, it has a different blocking mechanism for acquiring locks.
 * Just before acquiring a unique lock, the @c IntentAwareMutex expresses its intent to acquire that unique lock. This
 * prevents new shared locks from acquiring their lock and thus starves out shared locks before acquiring the unique
 * lock. This ensures a unique lock request will not have to keep waiting forever to acquire it while 'persistent'
 * shared locks would otherwise prevent that.
 *
 * @note This implementation satisfies the named requirement SharedMutex.
 * @see https://en.cppreference.com/w/cpp/named_req/SharedMutex
 * @author houthacker
 */
class IntentAwareMutex {
 private:
    std::shared_mutex unique_lock_intent_mutex;
    std::atomic_bool unique_lock_requested;

    std::condition_variable_any cv;

    std::shared_mutex mutex;

 public:

    IntentAwareMutex() =default;
    IntentAwareMutex(IntentAwareMutex&) =delete;
    IntentAwareMutex(IntentAwareMutex&&) noexcept =delete;

    ~IntentAwareMutex() =default;

    IntentAwareMutex& operator=(const IntentAwareMutex&) =delete;
    IntentAwareMutex& operator=(IntentAwareMutex&&) noexcept =delete;

    /**
     * @brief Acquires a unique lock on this mutex.
     * @details If another thread has a shared- or unique lock, this method blocks until the unique lock
     * can be acquired.
     */
    void lock() {
      // Indicate we're going to request the unique lock and notify all waiting threads of this.
      // This will also allow threads having a shared lock to finish up and release it, but deny new shared locks until
      // the unique lock that is being requested here is released again.
      {
        std::unique_lock<std::shared_mutex> lock(this->unique_lock_intent_mutex);
        this->unique_lock_requested = true;
      }
      this->cv.notify_all();

      // Block until the unique lock has been acquired.
      this->mutex.lock();

      // When the unique lock has been acquired, notify all waiting threads that the unique lock is not requested
      // anymore, allowing them to acquire a shared lock after this unique lock has been released later.
      {
        std::unique_lock<std::shared_mutex> lock(this->unique_lock_intent_mutex);
        this->unique_lock_requested = false;
      }
      this->cv.notify_all();
    }

    /**
     * @brief Tries to acquire the unique lock on this mutex.
     * @details If another thread has a shared- or unique lock, this method does not block but returns immediately.
     *
     * @return Whether the unique lock was acquired.
     */
    bool try_lock() {
      // Since this method must be non-blocking, we must not show our intent to acquire the unique lock, but just
      // get it if possible and bail out immediately if we can't own the lock right now.
      return this->mutex.try_lock();
    }

    /**
     * @brief Releases the unique lock. Releasing the lock when it is not held results in undefined behaviour.
     */
    void unlock() noexcept{
      this->mutex.unlock();
    }

    /**
     * @brief Acquires a shared lock for this mutex.
     * @details This method blocks until a possibly current unique lock is released and no other thread intends
     * to acquire the unique lock.
     */
    void lock_shared() {
      // Acquire a shared lock on the unique_lock_intent_mutex since multiple threads may want to get a shared lock
      // of course. If a unique lock has been requested by another thread, block until it has been released.
      std::shared_lock<std::shared_mutex> lock(this->unique_lock_intent_mutex);
      this->cv.wait(lock, [&]{return !this->unique_lock_requested; });

      // Now acquire the shared lock while having unique_lock_intent_mutex, since this prevents acquiring a unique lock on
      // the mutex used below.
      this->mutex.lock_shared();
    }

    /**
     * @brief Acquires a shared lock for this mutex.
     * @details Tries to acquire a shared lock on this mutex. If another thread has, or intends to have the unique lock,
     * this method returns immediately without acquiring the shared lock.
     *
     * @return Whether the shared lock was acquired.
     */
    bool try_lock_shared() {
      // Since this method must be non-blocking, just try to get a shared lock and bail out immediately if we can't
      // get a hold of it.
      return  !this->unique_lock_requested && this->mutex.try_lock_shared();
    }

    /**
     * @brief Releases the shared lock. Releasing the shared lock when it is not held results in undefined behaviour.
     */
    void unlock_shared() noexcept {
      this->mutex.unlock_shared();
    }
};

}

#endif //NOID_SRC_BACKEND_CONCURRENT_INTENTAWAREMUTEX_H_
