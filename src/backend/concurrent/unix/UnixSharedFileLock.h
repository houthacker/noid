/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_CONCURRENT_NOIDSHAREDFILELOCK_H_
#define NOID_SRC_BACKEND_CONCURRENT_NOIDSHAREDFILELOCK_H_

#include <cerrno>
#include <fcntl.h>
#include <source_location>

#include "backend/concurrent/Concepts.h"

namespace noid::backend::concurrent {

template<SharedMutex SharedMutex>
class UnixSharedFileLock {
 private:
    SharedMutex &mutex;

    int fd;
    std::thread::id mutex_holder;

 public:
    UnixSharedFileLock(SharedMutex &mutex, int fd) : mutex(mutex), fd(fd) {
      this->lock_shared();
    }

    UnixSharedFileLock(SharedMutex &mutex, int fd, std::defer_lock_t) : mutex(mutex), fd(fd) {}

    UnixSharedFileLock(SharedMutex &mutex, int fd, std::try_to_lock_t) : mutex(mutex), fd(fd) {
      this->try_lock_shared();
    }

    ~UnixSharedFileLock() {
      this->unlock_shared();
    };

    void lock_shared() {
      this->mutex.lock_shared();
      this->mutex_holder = std::this_thread::get_id();

      struct flock64 request = {
          .l_type     = F_RDLCK,  // request shared (read) lock
          .l_whence   = SEEK_SET, // positions are absolute
          .l_start    = 0,        // start at offset 0
          .l_len      = 0,        // lock the whole file
          .l_pid      = 0,        // using open file descriptor locks, so set PID to 0
      };

      // Try to acquire a lock. If this thread gets interrupted, keep trying until the lock has been acquired.
      while (fcntl64(this->fd, F_OFD_SETLKW /* blocking request */, &request) == -1) {
        auto ec = errno;
        if (ec != EINTR) {
          NOID_LOG_TRACE(std::source_location::current(),  core::api::GetErrorText(ec));

          // If locking fails due to another reason, give up and release the thread lock.
          this->unlock_shared();
          break;
        }
      }
    }

    bool try_lock_shared() {
      if (this->mutex.try_lock_shared()) {
        this->mutex_holder = std::this_thread::get_id();

        struct flock64 request = {
            .l_type     = F_RDLCK,  // request shared (read) lock
            .l_whence   = SEEK_SET, // positions are absolute
            .l_start    = 0,        // start at offset 0
            .l_len      = 0,        // lock the whole file
            .l_pid      = 0,        // using open file descriptor locks, so set PID to 0
        };

        if (fcntl64(this->fd, F_OFD_SETLK /* non-blocking request */, &request) == -1) {
          this->unlock_shared();
          return false;
        }

        return true;
      }

      return false;
    }

    void unlock_shared() {
      // We can only unlock the file lock if we own the mutex.
      if (this->mutex_holder==std::this_thread::get_id()) {
        struct flock64 request = {
            .l_type     = F_UNLCK,  // request file unlock
            .l_whence   = SEEK_SET, // positions are absolute
            .l_start    = 0,        // start at offset 0
            .l_len      = 0,        // lock the whole file
            .l_pid      = 0,        // using open file descriptor locks, so set PID to 0
        };

        // Since a shared lock is only acquired if we also share a file lock, unlocking _should_ not fail.
        if (fcntl64(this->fd, F_OFD_SETLK /* non-blocking request */, &request)==-1) {
          auto location = std::source_location::current();
          switch (errno) {
#ifdef EACCESS
            case EACCESS:
#else
            case EAGAIN:
#endif
              // EACCESS or EAGAIN is returned if there is a conflicting lock owned by another process.
              // Trying to unlock it is a bug, because the thread lock is only acquired if a shared file lock
              // has been acquired.
              NOID_LOG_WARN(location, "Trying to unlock file lock which is held by different process");
              errno = EPERM; // TODO We decide here it's not allowed, but is there a better error code?
              break;
            default:
              NOID_LOG_WARN(location, "Could not unlock file, but releasing thread mutex anyway.");
          }
        }

        // Reset the holder to a value that does not represent a thread.
        this->mutex_holder = std::thread::id();

        // Unlock the mutex, even if fcntl64 fails. Worst case: we keep the file lock, and release the memory lock.
        this->mutex.unlock_shared();
      }
    }
};

}
#endif //NOID_SRC_BACKEND_CONCURRENT_NOIDSHAREDFILELOCK_H_
