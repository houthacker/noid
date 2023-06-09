/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <catch2/catch.hpp>

#include <atomic>
#include <thread>

#include "backend/FixedSizeVector.h"
#include "backend/vfs/unix/UnixFile.h"
#include "backend/concurrent/Concepts.h"

using namespace noid::backend;
using namespace std::chrono_literals;

using noid::backend::vfs::UnixFile;

TEST_CASE("Can read and write a temporary UnixFile")
{
  FixedSizeVector<byte> data = {1, 3, 3, 7};

  auto temp_file = UnixFile::CreateTempFile();

  NoidLock auto lock = temp_file->UniqueLock();
  REQUIRE(temp_file->WriteContainer(data, 0) == data.size());
  temp_file->Flush();

  FixedSizeVector<byte> from_file(data.size());
  REQUIRE(temp_file->ReadContainer(from_file, 0, data.size()) == data.size());
  REQUIRE(data == from_file);
}

TEST_CASE("Can read and write a regular UnixFile")
{
  FixedSizeVector<byte> data = {1, 3, 3, 7};

  // Open a scoped file, so it gets deleted on exit or exception.
  auto regular_file = UnixFile::OpenScoped("noid_unixfile_test.ndb");

  NoidLock auto lock = regular_file->UniqueLock();
  REQUIRE(regular_file->WriteContainer(data, 0) == data.size());
  regular_file->Flush();

  FixedSizeVector<byte> from_file(data.size());
  REQUIRE(regular_file->ReadContainer(from_file, 0, data.size()) == data.size());
  REQUIRE(data == from_file);
}

TEST_CASE("Can retrieve the file size of a UnixFile") {
  FixedSizeVector<byte> data = {1, 3, 3, 7};

  // Open a scoped file, so it gets deleted on exit or exception.
  auto regular_file = UnixFile::OpenScoped("noid_unixfile_test.ndb");

  {
    NoidLock auto lock = regular_file->UniqueLock();
    REQUIRE(regular_file->WriteContainer(data, 0) == data.size());
    regular_file->Flush();
  }

  {
    NoidSharedLock auto lock = regular_file->SharedLock();
    REQUIRE(regular_file->Size() == data.size());
  }
}

TEST_CASE("Can read and write using a STL container")
{
  std::array<byte, 4> data = {1, 3, 3, 7};

  auto temp_file = UnixFile::CreateTempFile();

  NoidLock auto lock = temp_file->UniqueLock();
  REQUIRE(temp_file->WriteContainer(data, 0) == data.size());
  temp_file->Flush();

  std::array<byte, 4> from_file = {0};
  REQUIRE(temp_file->ReadContainer(from_file, 0) == data.size());
  REQUIRE(data == from_file);
}

TEST_CASE("OS file locks are preserved until the last call to close (2).")
{
#include <fcntl.h>
  // To test this, we open a UnixFile and acquire a unique lock on it.
  // This is a scoped file which is removed when it goes out of scope or if an exception is thrown.
  fs::path path = "noid_os_file_lock_test.ndb";
  auto regular_file = UnixFile::OpenScoped(path);
  NoidLock auto lock = regular_file->UniqueLock();

  // Then, in a different thread, we create a second UnixFile instance for the same file and destruct it immediately after.
  // This causes the locks on that particular file to be released, but must not release any other locks.
  {
    std::thread worker([path] {
      std::ignore = UnixFile::Open(path);
    });
    worker.join();
  }

  // Finally, we check if the OS file locks have been preserved while the first UnixFile is still in scope.
  // This must be a different file descriptor than the one from regular_file.
  // A unique lock on another UnixFile in a different thread will not work because UnixFile::UniqueLock() retrieves
  // an in-process lock on the file inode (which is already held), so we must use fcntl to retrieve the lock information.
  {
    // Put the file in a separate scope to ensure that regular_file gets destructed last.
    auto third_file = UnixFile::Open(path);

    struct flock64 request = {
        .l_type     = F_WRLCK,  // request unique (write) lock
        .l_whence   = SEEK_SET, // positions are absolute
        .l_start    = 0,        // start at offset 0
        .l_len      = 0,        // lock the whole file
        .l_pid      = 0,        // using open file descriptor locks, so set PID to 0
    };

    // Get lock information
    REQUIRE(fcntl64(third_file->GetFileDescriptor(), F_OFD_GETLK /* blocking request? */, &request) != -1);

    // If the unique lock could be placed, request.l_type must be F_UNLCK. Otherwise, request contains
    // information about the conflicting lock, which is what we expect. request.l_pid must be -1
    // since we're placing Open File Descriptor locks on these files.
    REQUIRE_FALSE(request.l_type == F_UNLCK);
    REQUIRE(request.l_pid == -1);
  }
}

TEST_CASE("UniqueLock-request drains SharedLocks")
{
  std::atomic_bool have_unique_lock = false;

  // Create a temporary file which will get removed after the test.
  auto file = UnixFile::CreateTempFile();

  // Start acquiring shared locks until the unique lock has been acquired.
  std::thread shared_locks([file, &have_unique_lock] {
    while (!have_unique_lock) {
      NoidSharedLock auto lock = file->SharedLock();
      std::this_thread::sleep_for(1s);
    }
  });

  // Let shared locks be acquired for some 0.5 seconds more.
  std::this_thread::sleep_for(0.5s);

  // Acquire the unique lock. This only succeeds if there are no shared locks.
  // To achieve this, the current shared locks must be drained, in accordance with the IntentAwareMutex.
  // TODO Implement and use TryUniqueLock(timeout)?
  std::thread unique_lock([file, &have_unique_lock] {
    NoidLock auto lock = file->UniqueLock();
    have_unique_lock = true;
  });

  // Wait on the threads to finish.
  shared_locks.join();
  unique_lock.join();
}
