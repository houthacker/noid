/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_STORAGE_PAGER_H_
#define NOID_SRC_STORAGE_PAGER_H_

#include <cstdint>
#include <filesystem>
#include <memory>

#include "backend/DynamicArray.h"
#include "backend/concurrent/Concepts.h"
#include "backend/page/Concepts.h"
#include "backend/vfs/NoidFile.h"
#include "backend/page/DatabaseHeader.h"

namespace fs = std::filesystem;
using namespace noid::backend::page;

namespace noid::backend {

/**
 * @brief The @c Pager is responsible for reading, writing and caching pages from the physical database file.
 *
 * @author houthacker
 */
template<Lockable Lockable, SharedLockable SharedLockable>
class Pager {
 private:

    /**
     * @brief The associated database file.
     */
    std::shared_ptr<vfs::NoidFile<Lockable, SharedLockable>> file;

    /**
     * @brief The page size this @c Pager uses.
     */
    uint16_t page_size;

    /**
     * @brief The maximum amount of times to retry a failed read or write action before (re)throwing and exception.
     * This value is currently hard-coded to 5.
     */
    uint8_t max_io_retries;

    /**
     * @brief Creates a new @c Pager which uses the given @p file.
     *
     * @param file The database file.
     */
    explicit Pager(std::shared_ptr<vfs::NoidFile<Lockable, SharedLockable>> file, uint16_t page_size)
        :file(std::move(file)), page_size(page_size), max_io_retries(3) { }

 public:
    ~Pager() = default;

    /**
     * @brief Returns a pager for the given database file.
     * @details If the database file is empty, it is initialized before returning the new @c Pager.
     *
     * @param file The database file to use.
     * @return A shared pointer to the newly created pager.
     * @throws std::ios_base::failure if the file cannot be opened, cannot be initialized or is not a noid database.
     */
    static std::unique_ptr<Pager<Lockable, SharedLockable>> Open(
        std::shared_ptr<vfs::NoidFile<Lockable, SharedLockable>> file)
    {

      {
        NoidLock unique_lock = file->UniqueLock();

        // Only initialize the database file if it is empty. Otherwise, if its size
        // is less (or greater) than page::DatabaseHeader::BYTE_SIZE, it could be that it is not a Noid database file at all.
        // In that case the @c Pager will throw after trying to read the header because it cannot read a valid header.
        // This leaves invalid database files intact.
        if (file->Size() == 0) {
          auto header = DatabaseHeader::NewBuilder()->Build();

          // TODO Write to WAL, not file directly.
          if (file->WriteContainer(header->ToBytes(), 0) != DatabaseHeader::BYTE_SIZE) {
            throw std::ios_base::failure("Cannot initialize database file.");
          }

          file->Flush();
          return std::unique_ptr<Pager<Lockable, SharedLockable>>(new Pager<Lockable, SharedLockable>(std::move(file), DEFAULT_PAGE_SIZE));
        }
      }

      // If we end up here, the database file existed already, and we must read its page size to configure the @c Pager.
      // We could also do this in the @c else branch of the above @c if statement, but then we'd keep the unique lock
      // longer than absolutely necessary.
      std::array<byte, DatabaseHeader::BYTE_SIZE> bytes = {0};
      {
        NoidSharedLock shared_lock = file->SharedLock();
        if (file->ReadContainer(bytes, 0) != DatabaseHeader::BYTE_SIZE) {
          throw std::ios_base::failure("Could not read database header.");
        }
      }

      auto page_size = DatabaseHeader::NewBuilder(bytes)->Build()->GetPageSize();
      return std::unique_ptr<Pager<Lockable, SharedLockable>>(new Pager<Lockable, SharedLockable>(std::move(file), page_size));
    }

    /**
     * @brief Reads the first page (index 0) from the database file.
     *
     * @return The database index page.
     * @throws std::filesystem::filesystem_error if the header cannot be read.
     */
    std::shared_ptr<const DatabaseHeader> ReadFileHeader()
    {
      std::array<byte, DatabaseHeader::BYTE_SIZE> bytes = {0};

      // TODO check WAL, page cache
      NoidSharedLock shared_lock = this->file->SharedLock();
      for (auto i = this->max_io_retries; i > 0; i--) {
        if (this->file->ReadContainer(bytes, 0) == DatabaseHeader::BYTE_SIZE) {
          return DatabaseHeader::NewBuilder(bytes)->Build();
        }
      }

      throw fs::filesystem_error(std::string("Cannot read header; retries exhausted."),
          std::make_error_code(std::errc::protocol_error));
    }

    /**
     * @brief Writes the header page (index 0) to the database file.
     *
     * @param header The database index page.
     * @throws std::filesystem::filesystem_error if an i/o error occurs while writing the header.
     */
    void WriteFileHeader(const DatabaseHeader& header)
    {

      // TODO write to WAL (, page cache?)
      NoidLock unique_lock = this->file->UniqueLock();
      for (auto i = this->max_io_retries; i > 0; i--) {
        if (this->file->Write(header.ToBytes(), 0) == DatabaseHeader::BYTE_SIZE) {
          return;
        }
      }

      throw fs::filesystem_error(std::string("Cannot write header; retries exhausted."),
          std::make_error_code(std::errc::protocol_error));
    }

    template<typename P, typename B> requires Page<P, B>
    std::unique_ptr<const P> RetrievePage(PageNumber location) {
      if (location == NO_PAGE) {
        throw std::out_of_range("Given page location designates 'no page'.");
      }

      Position file_pos = (location - 1) * this->page_size + DatabaseHeader::BYTE_SIZE;
      auto data = DynamicArray<byte>(this->page_size);

      // TODO check WAL first
      NoidSharedLock shared_lock = this->file->SharedLock();
      for (auto i = this->max_io_retries; i > 0; i--) {
        if (this->file->ReadContainer(data, file_pos, this->page_size) == this->page_size) {
          return P::NewBuilder(std::move(data))->Build();
        }
      }

      throw std::ios_base::failure("Cannot read page; retries exhausted.");
    }

    template<typename P, typename B> requires Page<P, B>
    void StorePage(const P& page, PageNumber location) {
      if (location == NO_PAGE) {
        throw std::out_of_range("Given page location designates 'no page'.");
      }

      Position page_index = (location - 1) * this->page_size + DatabaseHeader::BYTE_SIZE;

      // TODO write to WAL only
      NoidLock unique_lock = this->file->UniqueLock();
      for (auto i = this->max_io_retries; i > 0; i--) {
        auto data = page.ToBytes();
        if (data.size() == this->page_size) {
          if (this->file->WriteContainer(data, page_index) == page_size) {
            return;
          }
        } else {
          throw std::domain_error("Cannot write page because its size is not the configured page size.");
        }
      }

      throw std::ios_base::failure("Cannot write page; retries exhausted.");
    }
};

}

#endif //NOID_SRC_STORAGE_PAGER_H_
