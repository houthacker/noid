/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_VFS_UNIX_INODE_H_
#define NOID_SRC_BACKEND_VFS_UNIX_INODE_H_

#include <atomic>
#include <memory>
#include <shared_mutex>
#include <sys/stat.h>
#include <unordered_map>
#include <unordered_set>

#include "backend/concurrent/IntentAwareMutex.h"

using noid::backend::concurrent::IntentAwareMutex;

namespace noid::backend::vfs {

class Inode {
 private:

    /**
     * @brief The @c Inode number.
     */
    ino_t inode_number;

    /**
     * @brief The amount of references to this @c Inode
     */
    std::atomic_uint64_t ref_count;

    /**
     * @brief The associated file descriptors pending to be closed.
     */
    std::unordered_set<int> pending;

    /**
     * @brief Creates a new @c Inode instance with the given inode number.
     *
     * @param inode_number The inode number.
     */
    explicit Inode(ino_t inode_number);
 public:

    ~Inode() =default;

    /**
     * @brief Mutex to protect access to this inode.
     */
    IntentAwareMutex mutex;

    /**
     * @brief Retrieves the @c Inode instance associated with the given stream.
     *
     * @param stream The file to retrieve the @c Inode of.
     * @return The retrieved @c Inode
     * @throws std::ios_base::failure if the inode cannot be retrieved.
     */
    static std::shared_ptr<Inode> Of(std::FILE* stream);

    /**
     * @brief Closes the given file.
     * @details Since multiple @c std::FILE instances may point to the same inode but use different
     * file descriptors, closing the stream here just flushes the user-space buffers towards the kernel buffers,
     * decrements the reference count of this @c Inode instance and adds the associated file descriptor to the
     * pending (to close) list.
     * If no more @c std::FILE instances reference this @c Inode, all file descriptors in the pending list are closed
     * and the pending list is cleared. This puts the burden of closing all file descriptors on the final stream using
     * the inode. This should not be too much of an issue however, because nobody is writing to any of the files at
     * that point.
     *
     * @param stream The stream to close.
     * @return Whether closing the stream was successful. If unsuccessful, a retry results in undefined behaviour.
     * As such, the returned value is only useful for logging- and debugging purposes.
     */
    void Close(std::FILE* stream) noexcept;
};

}

#endif //NOID_SRC_BACKEND_VFS_UNIX_INODE_H_
