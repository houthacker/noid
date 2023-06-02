#include <array>
#include <filesystem>

#include "Pager.h"

namespace noid::backend {

template<Lockable Lockable, SharedLockable SharedLockable>
Pager<Lockable, SharedLockable>::Pager(std::shared_ptr<vfs::NoidFile<Lockable, SharedLockable>> file) : file(std::move(file)), max_io_retries(5) {}

template<Lockable Lockable, SharedLockable SharedLockable>
std::unique_ptr<Pager<Lockable, SharedLockable>> Pager<Lockable, SharedLockable>::Open(std::shared_ptr<vfs::NoidFile<Lockable, SharedLockable>> file) {
  return std::unique_ptr<Pager>(new Pager(std::move(file)));
}

template<Lockable Lockable, SharedLockable SharedLockable>
std::shared_ptr<const page::DatabaseHeader> Pager<Lockable, SharedLockable>::ReadFileHeader() {
  std::array<byte, page::DatabaseHeader::BYTE_SIZE> bytes = {0};

  // TODO check WAL, page cache
  NoidLock lock = this->file->SharedLock();
  for (auto i = this->max_io_retries; i > 0; i--) {
    if (this->file->Read(bytes, 0) == page::DatabaseHeader::BYTE_SIZE) {
      return page::DatabaseHeader::NewBuilder(bytes)->Build();
    }
  }

  throw fs::filesystem_error(std::string("Cannot read header; retries exhausted."),
                             std::make_error_code(std::errc::protocol_error));
}

template<Lockable Lockable, SharedLockable SharedLockable>
void Pager<Lockable, SharedLockable>::WriteFileHeader(const page::DatabaseHeader &header) {

  // TODO write to WAL (, page cache?)
  NoidLock lock = this->file->UniqueLock();
  for (auto i = this->max_io_retries; i > 0; i--) {
    if (this->file->Write(header.ToBytes(), 0) == page::DatabaseHeader::BYTE_SIZE) {
      return;
    }
  }

  throw fs::filesystem_error(std::string("Cannot write header; retries exhausted."),
                             std::make_error_code(std::errc::protocol_error));
}



}