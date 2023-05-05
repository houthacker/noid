/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_STORAGE_PAGER_H_
#define NOID_SRC_STORAGE_PAGER_H_

#include <filesystem>
#include <fstream>
#include <memory>

namespace fs = std::filesystem;


namespace noid::backend {

/**
 * @brief The @c Pager is responsible for reading, writing and caching pages from the physical database file.
 *
 * @author houthacker
 */
class Pager {
 private:

    /**
     * The associated database file.
     */
    std::fstream file;

    /**
     * @brief Creates a new @c Pager which uses the given @p file.
     *
     * @param file The database file.
     */
    explicit Pager(std::fstream file);

 public:
    ~Pager() = default;

    /**
     * @brief Creates a new pager for the database file at the given @p absolute_path.
     * @details If the database file does not exist, it is created and initialized.
     *
     * @param path The path to the database file to use.
     * @return A shared pointer to the newly created pager.
     */
    static std::shared_ptr<Pager> Open(const fs::path& path);
};

}

#endif //NOID_SRC_STORAGE_PAGER_H_
