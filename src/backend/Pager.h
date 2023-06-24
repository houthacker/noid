/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_STORAGE_PAGER_H_
#define NOID_SRC_STORAGE_PAGER_H_

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <sstream>
#include <utility> // std::pair

#include "backend/Concepts.h"
#include "backend/DynamicArray.h"
#include "backend/concurrent/Concepts.h"
#include "backend/page/Concepts.h"
#include "backend/vfs/NoidFile.h"
#include "backend/page/FileHeader.h"
#include "backend/page/LeafNode.h"
#include "core/api/Error.h"

namespace fs = std::filesystem;

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
     * This value is currently hard-coded to 3.
     */
    uint8_t max_io_retries = 3;

    /**
     * @brief Creates a new @c Pager which uses the given @p file.
     *
     * @param file The database file.
     */
    explicit Pager(std::shared_ptr<vfs::NoidFile<Lockable, SharedLockable>> file, uint16_t page_size)
        :file(std::move(file)), page_size(page_size) { }

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
        // is less (or greater) than page::FileHeader::SIZE, it could be that it is not a Noid database file at all.
        // In that case the @c Pager will throw after trying to read the header because it cannot read a valid header.
        // This leaves invalid database files intact.
        if (file->Size() == 0) {

          // TODO Write to WAL, not file directly.
          if (auto header = page::FileHeader::NewBuilder()->Build(); file->WriteContainer(header->ToBytes(), 0) != page::FileHeader::SIZE) {
            throw std::ios_base::failure("Cannot initialize database file.");
          }

          file->Flush();
          return std::unique_ptr<Pager<Lockable, SharedLockable>>(
              new Pager<Lockable, SharedLockable>(std::move(file), DEFAULT_PAGE_SIZE));
        }
      }

      // If we end up here, the database file existed already, and we must read its page size to configure the @c Pager.
      // We could also do this in the @c else branch of the above @c if statement, but then we'd keep the unique lock
      // longer than absolutely necessary.
      std::array<byte, page::FileHeader::SIZE> bytes = {0};
      {
        NoidSharedLock shared_lock = file->SharedLock();
        // TODO read from WAL, or catch-22?
        if (file->ReadContainer(bytes, 0) != page::FileHeader::SIZE) {
          throw std::ios_base::failure("Could not read database header.");
        }
      }

      auto page_size = page::FileHeader::NewBuilder(bytes)->Build()->GetPageSize();
      return std::unique_ptr<Pager<Lockable, SharedLockable>>(
          new Pager<Lockable, SharedLockable>(std::move(file), page_size));
    }

    /**
     * @brief Calculates the amount of @c Overflow pages required to fit @p value.
     *
     * @param value The data to calculate the amount of required @c Overflow pages for.
     * @return The amount of required @c Overflow pages.
     */
    uint32_t CalculateOverflow(const V& value)
    {
      return page::NodeRecord::CalculateOverflow(value, this->page_size);
    }

    /**
     * @brief Lazily allocates new contiguous space for @p count pages.
     * @details Since this method returns the page numbers assigned to the new allocated space, this method can be
     * used to 'reserve' a range of page numbers to write to at a later time. This is useful for example when writing
     * data to a leaf node which will overflow, because the required space can be calculated and then reserved to
     * ensure contiguous storage of related data.<br />
     * Allocating lazily means that the underlying file is grown so new page numbers can be assigned, but only actually
     * written to when the pages are committed to the file. This can cause delayed errors if the device is full
     * when writing.
     *
     * @see @c noid::backend::vfs::NoidFile::Grow(uint32_t)
     *
     * @param count The amount of pages to allocate.
     * @return The page number range @c first (inclusive), @c last (exclusive).
     * @throws std::domain_error if the database file is not aligned to @c Pager::page_size.
     * @throws std::ios_base::failure if allocating the requested space fails.
     */
    std::pair<PageNumber, PageNumber> AllocateContiguous(uint32_t count)
    {
      if (count == 0) {
        return std::make_pair(NULL_PAGE, NULL_PAGE);
      }

      NoidLock unique_lock = this->file->UniqueLock();
      auto file_size = this->file->Size();

      // Only return the page number range if the file size is aligned to full pages.
      if (auto page_fraction = 1.0 * (file_size - page::FileHeader::SIZE) / this->page_size; page_fraction == std::floor(page_fraction)) {
        auto allocation_range_start = static_cast<PageNumber>(page_fraction);
        auto allocation_range_end = allocation_range_start + count;

        // Now increase the file size
        this->file->Grow(static_cast<uintmax_t>(count) * this->page_size);

        return std::make_pair(allocation_range_start, allocation_range_end);
      }

      // Throwing this exception essentially prevents vacuuming (moving WAL-pages into the database file)
      // since no new pages can be allocated.
      throw std::domain_error("Page allocation failed: database file is not aligned to page size.");
    }

    /**
     * @brief Reads the first page (index 0) from the database file.
     *
     * @return The database index page.
     * @throws std::ios_base::failure if the header cannot be read.
     */
    std::shared_ptr<const page::FileHeader> ReadFileHeader()
    {
      std::array<byte, page::FileHeader::SIZE> bytes = {0};

      // TODO check WAL, page cache
      NoidSharedLock shared_lock = this->file->SharedLock();
      for (auto i = this->max_io_retries; i > 0; i--) {
        if (this->file->ReadContainer(bytes, 0) == page::FileHeader::SIZE) {
          return page::FileHeader::NewBuilder(bytes)->Build();
        }
      }

      throw std::ios_base::failure(std::string("Cannot read header; retries exhausted."));
    }

    /**
     * @brief Writes the header page (index 0) to the database file and flushes the user-space buffers.
     *
     * @param header The database index page.
     * @throws std::ios_base::failure if an i/o error occurs while writing the header.
     */
    void WriteFileHeader(const page::FileHeader& header)
    {

      // TODO write to WAL (, page cache?)
      NoidLock unique_lock = this->file->UniqueLock();
      for (auto i = this->max_io_retries; i > 0; i--) {
        if (this->file->Write(header.ToBytes(), 0) == page::FileHeader::SIZE) {
          return this->file->Flush();
        }
      }

      throw std::ios_base::failure(std::string("Cannot write header; retries exhausted."));
    }

    /**
     * @brief Creates a new builder for page type @c P and returns it with the page size set.
     *
     * @tparam B The builder type.
     * @tparam P The page type.
     * @return A unique pointer to the builder instance.
     */
    template<typename P, typename B> requires page::Page<P, B>
    std::shared_ptr<B> NewBuilder()
    {
      return P::NewBuilder(this->page_size);
    }

    /**
     * @brief Retrieves the page at the given location from storage.
     * @details This method blocks until a shared lock on the associated database file can be acquired and then
     * tries to read the page at the given location. If the read fails, this method retries <code>Page::max_io_retries - 1</code>
     * times. After that, it gives up and throws an exception.
     *
     * @tparam P The page type.
     * @tparam B The builder type to build pages of type @c P.
     * @param location The location at which the page resides.
     * @return The page at the given location.
     * @throws std::ios_base::failure if the page cannot be read.
     */
    template<typename P, typename B>
    requires page::Page<P, B>
    std::unique_ptr<const P> ReadPage(PageNumber location)
    {
      if (location == NULL_PAGE) {
        throw std::out_of_range("Given location references a null page.");
      }

      Position file_pos = (location - 1) * this->page_size + page::FileHeader::SIZE;
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

    /**
     * @brief Writes the given page to storage at the given location.
     * @details This method blocks until a unique lock on the associated database file has been acquired and then tries
     * to write the page at the given location. If the write fails, this method retries <code>Page::max_io_retries - 1</code>
     * times. After that, it gives up and throws and exception.
     *
     * @tparam P The page type.
     * @tparam B The builder type with which to build pages of type @c P.
     * @param page The page to write to storage.
     * @param location The location at which the page must reside.
     * @throws std::domain_error if the page size is not the configured page size.
     * @throws std::ios_base::failure if the page cannot be written.
     * @throws std::out_of_range if @p location equals @c noid::backend::NULL_PAGE
     */
    template<typename P, typename B>
    requires page::Page<P, B>
    void WritePage(const P& page, PageNumber location)
    {
      if (location == NULL_PAGE) {
        throw std::out_of_range("Given location references a null page.");
      }

      Position page_index = (location - 1) * this->page_size + page::FileHeader::SIZE;
      auto data = page.ToBytes();

      // TODO write to WAL only
      NoidLock unique_lock = this->file->UniqueLock();
      for (auto i = this->max_io_retries; i > 0; i--) {
        if (data.size() == this->page_size) {
          if (this->file->WriteContainer(data, page_index) == page_size) {
            return this->file->Flush();
          }
        }
        else {
          throw std::domain_error("Cannot write page because its size is not the configured page size.");
        }
      }

      throw std::ios_base::failure("Cannot write page; retries exhausted.");
    }

    /**
     * @brief Writes the given page to storage and returns the @c PageNumber at which it resides.
     *
     * @tparam P The page type.
     * @tparam B The builder type with which to build pages of type @c P.
     * @param page The page to write to storage.
     * @return The page number at which the given page resides.
     * @throws std::ios_base::failure if the page cannot be written.
     * @throws std::domain_error if the page size is not the configured page size, or if the storage itself is not
     * aligned to the page size.
     */
    template<typename P, typename B>
    requires page::Page<P, B>
    PageNumber WritePage(const P& page)
    {
      auto data = page.ToBytes();

      // TODO write to WAL only
      NoidLock unique_lock = this->file->UniqueLock();
      auto file_size = this->file->Size();

      // Check if the file size is aligned to this->page_size. If not, this is an indication of file corruption.
      if (auto page_fraction = 1.0 * (file_size - page::FileHeader::SIZE) / this->page_size; page_fraction == std::floor(page_fraction)) {
        for (auto i = this->max_io_retries; i > 0; i--) {
          if (data.size() == this->page_size) {
            if (this->file->WriteContainer(data, static_cast<Position>(file_size)) == this->page_size) {
              this->file->Flush();
              return static_cast<PageNumber>(page_fraction + 1);
            }
          }
          else {
            throw std::domain_error("Cannot write page because its size is not the configured page size.");
          }
        }

        throw std::ios_base::failure("Cannot write page; retries exhausted.");
      }

      std::stringstream stream;
      stream << "Cannot write page: file size is not aligned to page_size of " << +this->page_size << ".";
      throw std::domain_error(stream.str());
    }
};

}

#endif //NOID_SRC_STORAGE_PAGER_H_
