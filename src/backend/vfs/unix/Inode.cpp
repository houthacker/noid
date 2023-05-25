/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <cstdio>
#include <cerrno> // errno
#include <ios>
#include <mutex>
#include <optional>
#include <source_location>
#include <string>
#include <system_error>
#include <unistd.h> // close(), fsync()
#include <unordered_map>

#include "Inode.h"
#include "core/api/Error.h"
#include "backend/NoidConfig.h"

namespace noid::backend::vfs {

/**
 * This mutex is used for locking the static list of inodes.
 */
static std::mutex get_inode_mutex;

/**
 * @brief Map to keep track of all Inode instances of this template type.
 */
static std::unordered_map<ino_t, std::weak_ptr<Inode>> INODES;

static std::optional<ino_t> GetInodeNumber_(int fd) noexcept {
  if (fd) {
    struct stat info = {0};
    if (fstat(fd, &info) == 0) {
      return info.st_ino;
    }

    NOID_LOG_TRACE(std::source_location::current(), core::api::GetErrorText(errno));
  }

  return std::nullopt;
}

Inode::Inode(ino_t inode_number) : inode_number(inode_number), ref_count(1) {}

std::shared_ptr<Inode> Inode::Of(std::FILE *stream) {
  auto ino_opt = GetInodeNumber_(fileno(stream));

  if (ino_opt.has_value()) {
    auto ino = ino_opt.value();

    std::scoped_lock<std::mutex> inode_list_lock{get_inode_mutex};
    auto it = INODES.find(ino);
    if (it==INODES.end() || it->second.expired()) {
      auto inode = std::shared_ptr<Inode>(new Inode(ino));
      INODES[ino] = inode;

      return inode;
    }

    auto inode = it->second.lock(); // convert weak_ptr o shared_ptr
    inode->ref_count++;

    return inode;
  }

  throw std::ios_base::failure("Cannot read file metadata", std::make_error_code(std::errc::io_error));
}

void Inode::Close(std::FILE *stream) noexcept {
  auto fd = fileno(stream);
  auto ino_opt = GetInodeNumber_(fd);
  if (ino_opt.has_value() && ino_opt.value() == this->inode_number) {

    try {
      this->ref_count--;

      // If this Inode has no more references, this fd will get closed (and therefore flushed). But we don't know
      // when that will be, so the stream flushes its user-space buffers here.
      if (fflush(stream)==EOF) {
        // If that fails, just issue a warning and don't throw an exception. When this->ref_count reaches zero
        // the associated file descriptor will be flushed and closed.
        NOID_LOG_WARN(std::source_location::current(), "Cannot flush stream, retrying when stream gets closed.");
      }

      // Always add the file descriptor to the pending list, since it might not get closed otherwise.
      this->pending.insert(fd);

      // Close all pending file descriptors
      if (this->ref_count==0) {
        for (auto it : this->pending) {
          if (close(it)==-1) {
            NOID_LOG_TRACE(std::source_location::current(), "Cannot close file descriptor.");
          }
        }
        this->pending.clear();
      }
    } catch (const std::system_error& ex) {
      NOID_LOG_WARN(std::source_location::current(), "Could not obtain write lock");
    }
  } else {
    NOID_LOG_WARN(std::source_location::current(), "Ignoring: file descriptor of given std::FILE is not associated with this inode");
  }
}

}