/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_VFS_UNIX_UNIXVFS_H_
#define NOID_SRC_BACKEND_VFS_UNIX_UNIXVFS_H_

#include "backend/vfs/NoidVFS.h"
#include "backend/vfs/NoidFile.h"
#include "backend/concurrent/IntentAwareMutex.h"
#include "backend/concurrent/unix/UnixFileLock.h"
#include "backend/concurrent/unix/UnixSharedFileLock.h"
#include "UnixFile.h"

using namespace noid::backend::concurrent;

namespace noid::backend::vfs {

class UnixVFS : public NoidVFS<UnixFileLock<IntentAwareMutex>, UnixSharedFileLock<IntentAwareMutex>> {
 public:
    UnixVFS() =default;
    ~UnixVFS() override =default;

    /**
     * @brief Creates a new @c UnixFile and opens it. If the file does not exist, it is created.
     *
     * @param path The path to the file.
     * @return The opened file.
     * @throws std::filesystem::filesystem_error if opening the file fails.
     */
    std::shared_ptr<NoidFile<UnixFileLock<IntentAwareMutex>, UnixSharedFileLock<IntentAwareMutex>>> Open(const fs::path& path) override;

    /**
     * @brief Creates a temporary @c UnixFile and returns it. The file is deleted after the @c UnixFile is destructed.
     *
     * @return The temporary file.
     * @throws std::filesystem::filesystem_error if creating and opening the temporary file fails.
     */
    std::shared_ptr<NoidFile<UnixFileLock<IntentAwareMutex>, UnixSharedFileLock<IntentAwareMutex>>> CreateTempFile() override;
};

}

#endif //NOID_SRC_BACKEND_VFS_UNIX_UNIXVFS_H_
