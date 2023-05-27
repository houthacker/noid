/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include <cerrno>
#include <cstdio>
#include <dirent.h> // opendir, dirfd
#include <filesystem>
#include <mutex>
#include <source_location>
#include <sstream>
#include <sys/types.h> // opendir, dirfd
#include <unistd.h> // fdatasync, readlink

#include "core/api/Error.h"
#include "UnixFile.h"

namespace noid::backend::vfs {

static const uint8_t FFLUSH_OK = 0;
static const uint8_t FSYNC_OK = 0;

static std::FILE *GetValidFile(std::FILE *fp) {
  if (fp) {
    return fp;
  }

  throw std::ios_base::failure(core::api::GetErrorText(errno),
                               std::make_error_code(std::errc::io_error));
}

static int GetAndClearError(std::FILE *file) {
  auto ec = errno;
  clearerr(file);

  return ec;
}

static int GetDirectoryFileDescriptor(int fd) {
  std::stringstream stream("/proc/self/fd/");
  stream << +fd;

  char buf[PATH_MAX + 1] = {0};
  if (readlink(stream.str().c_str(), buf, PATH_MAX)!=-1) {
    auto directory = fs::path(buf).parent_path().c_str();
    auto dir_stream = opendir(directory);

    if (dir_stream) {
      return dirfd(dir_stream);
    }
  }

  return C_API_ERROR;
}

UnixFile::UnixFile(std::FILE *file)
    : file(file), fd(fileno(file)), buffer(std::make_unique<std::array<char, BUFSIZ>>()) {
  std::setbuf(this->file, this->buffer->data());
  this->inode = Inode::Of(this->file);
}

std::shared_ptr<UnixFile> UnixFile::Open(const fs::path &path) {
  return std::shared_ptr<UnixFile>(new UnixFile(GetValidFile(std::fopen(path.c_str(), "a+"))));
}

std::shared_ptr<UnixFile> UnixFile::OpenScoped(const fs::path &path) {
  return {new UnixFile(GetValidFile(std::fopen(path.c_str(), "a+"))),
          [path](auto p) {
            std::error_code code;
            if (!fs::remove(path, code)) {
              NOID_LOG_ERROR(std::source_location::current(), "Could not delete file!");
            }

            delete p;
          }};
}

std::shared_ptr<UnixFile> UnixFile::CreateTempFile() {
  return std::shared_ptr<UnixFile>(new UnixFile(GetValidFile(std::tmpfile())));
}

#ifdef NOID_TEST_BUILD
int UnixFile::GetFileDescriptor() {
  return this->fd;
}
#endif

std::size_t UnixFile::Write(const byte *source, Position start_position, std::size_t size) {
  if (fseeko64(this->file, start_position, SEEK_SET)==C_API_ERROR) {
    throw std::ios_base::failure(core::api::GetErrorText(errno),
                                 std::make_error_code(std::errc::io_error));
  }

  auto items_written = fwrite(source, size, 1, this->file);
  if (ferror(this->file)) {
    auto error_code = GetAndClearError(this->file);

    if (error_code==EINTR) {
      // If the thread got interrupted while writing to the file, just return the amount of bytes read,
      // allowing callers to retry.
      return items_written*size;
    }

    throw std::ios_base::failure(core::api::GetErrorText(error_code),
                                 std::make_error_code(std::errc::io_error));
  }

  return items_written*size;
}

std::size_t UnixFile::Read(std::vector<byte> &container,
                           std::vector<byte>::iterator container_pos,
                           Position start_position,
                           const std::size_t size) {
  if (fseeko64(this->file, start_position, SEEK_SET)==C_API_ERROR) {
    throw std::ios_base::failure(core::api::GetErrorText(errno),
                                 std::make_error_code(std::errc::io_error));
  }

  // TODO let file_buffer be the default page size
  std::array<byte, 4096> file_buffer = {0};
  auto remaining = size;

  // The following loop is slow if the std::FILE is unbuffered (hence UnixFile::buffer).
  while (remaining > 0) {
    fread(file_buffer.data(), file_buffer.size(), 1, this->file);

    if (ferror(this->file)) {
      auto error_code = GetAndClearError(this->file);

      if (error_code==EINTR) {
        // If the thread got interrupted while reading from the file, just return the amount of bytes read,
        // allowing callers to retry.
        return size - remaining;
      }

      throw std::ios_base::failure(core::api::GetErrorText(error_code),
                                   std::make_error_code(std::errc::io_error));
    }

    if (feof(this->file)) {
      clearerr(this->file);

      if (remaining < file_buffer.size()) {
        // We read the last part of the requested block
        container.insert(container_pos, std::begin(file_buffer), std::begin(file_buffer) + remaining);

        // Ensure the correct amount of read bytes are returned.
        remaining = 0;
      } else {

        // We ran into a premature end of the file. Do not insert the half-read block in @c container and issue a
        // warning.
        NOID_LOG_WARN(std::source_location::current(),
                      "Reached end of file before requested amount of bytes have been read.");

        // Breaking out of the loop next returns the remaining bytes without subtracting the current block size
        // indicating not all requested bytes have been read.
      }

      break; // while(remaining > 0)
    } else {
      container.insert(container_pos, std::begin(file_buffer), std::end(file_buffer));
      container_pos += file_buffer.size();

      remaining -= file_buffer.size();
    }
  }

  return size - remaining;
}

std::size_t UnixFile::Read(byte *destination, Position start_position, std::size_t size) {
  if (fseeko64(this->file, start_position, SEEK_SET)==C_API_ERROR) {
    throw std::ios_base::failure(core::api::GetErrorText(errno),
                                 std::make_error_code(std::errc::io_error));
  }

  auto items_read = fread(destination, size, 1, this->file);
  if (ferror(this->file)) {
    auto error_code = GetAndClearError(this->file);

    if (error_code==EINTR) {
      // If the thread got interrupted while reading from the file, just return the amount of bytes read,
      // allowing callers to retry.
      return items_read*size;
    }

    throw std::ios_base::failure(core::api::GetErrorText(error_code),
                                 std::make_error_code(std::errc::io_error));
  } else if (feof(this->file)) {
    clearerr(this->file);
    NOID_LOG_WARN(std::source_location::current(),
                  "Reached end of file before requested amount of bytes have been read.");
  }

  return items_read*size;
}

void UnixFile::Flush() {
  auto status_code = fflush(this->file);

  // The fflush documentation does not specify whether fflush sets the error indicator on the file stream. Clear it, just to be sure.
  clearerr(this->file);

  if (status_code!=FFLUSH_OK) {
    throw std::ios_base::failure(core::api::GetErrorText(status_code),
                                 std::make_error_code(std::errc::io_error));
  }
}

void UnixFile::Sync() {
  if (fdatasync(this->fd)==FSYNC_OK) {
    auto dir_fd = GetDirectoryFileDescriptor(this->fd);

    if (dir_fd!=C_API_ERROR) {

      // The fdatasync documentation states that calling fdatasync on just the file fd does not ensure that
      // the entry in the directory containing the file has also reached disk. To ensure that as well, call fdatasync
      // on the directory fd too.
      if (fdatasync(dir_fd)==FSYNC_OK) {
        return;
      }
    }
  }

  throw std::ios_base::failure(core::api::GetErrorText(errno),
                               std::make_error_code(std::errc::io_error));
}

UnixFileLock<IntentAwareMutex> UnixFile::UniqueLock() {
  return {this->inode->mutex, this->fd};
}

UnixFileLock<IntentAwareMutex> UnixFile::TryUniqueLock() {
  return {this->inode->mutex, this->fd, std::try_to_lock};
}

UnixSharedFileLock<IntentAwareMutex> UnixFile::SharedLock() {
  return {this->inode->mutex, this->fd};
}

UnixSharedFileLock<IntentAwareMutex> UnixFile::TrySharedLock() {
  return {this->inode->mutex, this->fd, std::try_to_lock};
}

UnixFile::~UnixFile() {
  this->inode->Close(this->file);
}

}