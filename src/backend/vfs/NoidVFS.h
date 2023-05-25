/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */


#ifndef NOID_SRC_BACKEND_VFS_NOIDVFS_H_
#define NOID_SRC_BACKEND_VFS_NOIDVFS_H_

#include <filesystem>
#include <memory>

#include "NoidFile.h"
#include "backend/concurrent/Concepts.h"

namespace fs = std::filesystem;

namespace noid::backend::vfs {

template<concurrent::Lockable Lockable, concurrent::SharedLockable SharedLockable>
class NoidVFS {
 public:
    virtual ~NoidVFS() =default;

    /**
     * @brief Creates a new @c NoidFile and opens it. If the file does not exist, it is created.
     *
     * @param path The path to the file.
     * @return The opened file.
     * @throws std::filesystem::filesystem_error if opening the file fails.
     */
    virtual std::shared_ptr<NoidFile<Lockable, SharedLockable>> Open(const fs::path& path) =0;

    /**
     * @brief Creates a temporary @c NoidFile and returns it. The file is deleted after the @c NoidFile is destructed.
     *
     * @return The temporary file.
     * @throws std::filesystem::filesystem_error if creating and opening the temporary file fails.
     */
    virtual std::shared_ptr<NoidFile<Lockable, SharedLockable>> CreateTempFile() =0;
};

}

#endif //NOID_SRC_BACKEND_VFS_NOIDVFS_H_
