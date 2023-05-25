/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_STORAGE_PAGER_H_
#define NOID_SRC_STORAGE_PAGER_H_

#include <cstdint>
#include <filesystem>
#include <memory>

#include "backend/concurrent/Concepts.h"
#include "backend/vfs/NoidFile.h"
#include "backend/page/DatabaseHeader.h"

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
     * @brief The maximum amount of times to retry a failed read or write action before (re)throwing and exception.
     * This value is currently hard-coded to 5.
     */
    uint8_t max_io_retries;

    /**
     * @brief Creates a new @c Pager which uses the given @p file.
     *
     * @param file The database file.
     */
    explicit Pager(std::shared_ptr<vfs::NoidFile<Lockable, SharedLockable>> file);

 public:
    ~Pager() =default;

    /**
     * @brief Returns a pager for the given database file.
     * @details If the database file does not exist, it is created and initialized.
     *
     * @param file The database file to use.
     * @return A shared pointer to the newly created pager.
     */
    static std::unique_ptr<Pager> Open(std::shared_ptr<vfs::NoidFile<Lockable, SharedLockable>> file);

    /**
     * @brief Reads the first page (index 0) from the database file.
     *
     * @return The database index page.
     * @throws std::filesystem::filesystem_error if the header cannot be read.
     */
    std::shared_ptr<const page::DatabaseHeader> ReadFileHeader();

    /**
     * @brief Writes the header page (index 0) to the database file.
     *
     * @param header The database index page.
     * @throws std::filesystem::filesystem_error if an i/o error occurs while writing the header.
     */
    void WriteFileHeader(const page::DatabaseHeader& header);
};

}

#endif //NOID_SRC_STORAGE_PAGER_H_
