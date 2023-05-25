/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */
#include <atomic>
#include <thread>
#include "backend/vfs/unix/UnixFile.h"
#include "backend/concurrent/Concepts.h"
#include <catch2/catch.hpp>

using namespace noid::backend;
using namespace std::chrono_literals;

static void acquire_shared_locks(std::shared_ptr<vfs::UnixFile> file, std::atomic_bool &have_unique_lock) {
  while (!have_unique_lock) {
    NoidSharedLock auto lock = file->SharedLock();
  }
}

static void acquire_unique_lock(std::shared_ptr<vfs::UnixFile> file, std::atomic_bool &have_unique_lock) {
  NoidLock auto lock = file->UniqueLock();
  have_unique_lock = true;
}

TEST_CASE("Can read and write a temporary UnixFile") {
  std::vector<byte> data = {1, 3, 3, 7};

  auto temp_file = vfs::UnixFile::CreateTempFile();

  NoidLock auto lock = temp_file->UniqueLock();
  REQUIRE(temp_file->WriteContainer(data, 0)==data.size());
  temp_file->Flush();

  std::vector<byte> from_file;
  from_file.reserve(data.size()); // Expect to read 4 bytes from the file
  REQUIRE(temp_file->ReadContainer(from_file, 0, data.size())==data.size());
  REQUIRE(data==from_file);
}

TEST_CASE("Can read and write a regular UnixFile") {
  std::vector<byte> data = {1, 3, 3, 7};

  // Open a scoped file, so it gets deleted on exit or exception.
  auto regular_file = vfs::UnixFile::OpenScoped("noid_unixfile_test.ndb");

  NoidLock auto lock = regular_file->UniqueLock();
  REQUIRE(regular_file->WriteContainer(data, 0)==data.size());
  regular_file->Flush();

  std::vector<byte> from_file;
  from_file.reserve(data.size()); // Expect to read 4 bytes from the file
  REQUIRE(regular_file->ReadContainer(from_file, 0, data.size())==data.size());
  REQUIRE(data==from_file);
}

TEST_CASE("Can read and write using a std::array<>") {
  std::array<byte, 4> data = {1, 3, 3, 7};

  auto temp_file = vfs::UnixFile::CreateTempFile();

  NoidLock auto lock = temp_file->UniqueLock();
  REQUIRE(temp_file->WriteContainer(data, 0)==data.size());
  temp_file->Flush();

  std::array<byte, 4> from_file = {0};
  REQUIRE(temp_file->ReadContainer(from_file, 0)==data.size());
  REQUIRE(data==from_file);
}

TEST_CASE("UniqueLock-request drains SharedLocks") {
  std::atomic_bool have_unique_lock = false;

  auto file = vfs::UnixFile::CreateTempFile();

  std::thread shared_locks(acquire_shared_locks, file, std::ref(have_unique_lock));

  std::this_thread::sleep_for(1s);
  std::thread unique_lock(acquire_unique_lock, file, std::ref(have_unique_lock));

  // Wait on the threads to finish.
  shared_locks.join();
  unique_lock.join();
}
