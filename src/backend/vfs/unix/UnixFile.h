/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_VFS_UNIX_UNIXFILE_H_
#define NOID_SRC_BACKEND_VFS_UNIX_UNIXFILE_H_

#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>

#include "backend/DynamicArray.h"
#include "backend/vfs/NoidFile.h"
#include "backend/vfs/unix/Inode.h"
#include "backend/concurrent/IntentAwareMutex.h"
#include "backend/concurrent/unix/UnixFileLock.h"
#include "backend/concurrent/unix/UnixSharedFileLock.h"

namespace fs = std::filesystem;
using namespace noid::backend::concurrent;

namespace noid::backend::vfs {

/**
 * @brief Binary file implementation for the unix VFS.
 *
 * @details This class is an abstraction over C-style I/O file r/w access. In the future, this might change
 * to a wrapper for stream-based I/O, but currently it is not easily possible to use the fine-grained locking like
 * in @c fcntl() in streams.
 *
 * @author houthacker
 */
class UnixFile : public NoidFile<UnixFileLock<IntentAwareMutex>, UnixSharedFileLock<IntentAwareMutex>> {
 private:

    /**
     * @brief The file stream to use for reading and writing.
     */
    std::FILE* file;

    /**
     * @brief The file descriptor associated with the file stream.
     */
    int fd;

    /**
     * @brief The in-memory buffer of the associated file.
     */
    std::unique_ptr<std::array<char, BUFSIZ>> buffer;

    /**
     * @brief The @c Inode used by the file stream.
     */
    std::shared_ptr<Inode> inode;

    explicit UnixFile(std::FILE* file);

 public:

    /**
     * @brief Creates a new @c UnixFile and opens it. If the file does not exist, it is created.
     *
     * @param path The path to the file.
     * @return The opened file.
     * @throws std::filesystem::filesystem_error if opening the file fails.
     */
    static std::shared_ptr<UnixFile> Open(const fs::path &path);

    /**
     * @brief Creates a new @c UnixFile and opens it. If ths file does not exist, it is created. After the pointer goes
     * out of scope, the file is deleted, silently ignoring any exceptions while doing so.
     *
     * @param path The path to the file.
     * @return The opened file.
     * @throws std::filesystem::filesystem_error if opening the file fails.
     */
    static std::shared_ptr<UnixFile> OpenScoped(const fs::path &path);

    /**
     * @brief Creates a temporary @c UnixFile and returns it. The file is deleted after the @c UnixFile is destructed.
     *
     * @return The temporary file.
     * @throws std::filesystem::filesystem_error if creating and opening the temporary file fails.
     */
    static std::shared_ptr<UnixFile> CreateTempFile();

#ifdef NOID_TEST_BUILD
    /**
     * @return The associated file descriptor.
     */
    [[nodiscard]] int GetFileDescriptor() const;
#endif

    /**
     * @brief Returns the file size in bytes.
     * @details This method does not block. This allows callers to decide themselves about the blocking scope.
     *
     * @return The file size.
     * @throw std::ios_base::failure if the file size could not be retrieved.
     */
    std::uintmax_t Size() override;

    /**
     * @brief Atomically writes the @p length bytes from @p source to this file, starting at @p start_position.
     *
     * @note This method does not block calling threads. This must be done separately to allow for consecutive calls
     * to @c UnixFile::Write() by a single thread.
     *
     * @warning Using this method without concurrency measures in effect will almost certainly cause file corruption.
     *
     * @param source The pointer to the source data.
     * @param start_position The position in this file to start writing at.
     * @param size The amount of bytes to write.
     * @return The amount of bytes written.
     * @throws std::ios_base::failure if writing @p size bytes to the file fails.
     */
    std::size_t Write(const byte* source, Position start_position, std::size_t size) override;

    /**
     * @brief Reads @p size bytes from this file into @p container, starting at @p start_position.
     *
     * @note This method does <em>not</em> insert element, but instead overwrites them and assumes the given container
     * can fit @p size elements from @p container_pos.
     *
     * @note Reading an arbitrary amount of bytes from a file is not atomic in the sense that all or nothing is read,
     * therefore this method is marked @c [[nodiscard]] to warn callers if they discard the actual amount of bytes read.
     *
     * @note This method does not block calling threads. This must be handled separately to allow for consecutive calls
     * to @c NoidFile::Read() by a single thread.
     *
     * @param container The vector to read into.
     * @param container_pos The pointer to the destination data.
     * @param start_position The position in this file to start reading at.
     * @param size The amount of bytes to read.
     * @return The amount of bytes read.
     * @throws std::ios_base::failure if reading @p size bytes from the file fails.
     */
    [[nodiscard]] std::size_t Read(DynamicArray<byte>& container, DynamicArray<byte>::iterator container_pos,
                                   Position start_position, std::size_t size) override;

    /**
     * @brief Reads @p size bytes from this file into @p destination, starting at @p start_position.
     *
     * @note Reading an arbitrary amount of bytes from a file is not atomic in the sense that all or nothing is read,
     * therefore this method is marked @c [[nodiscard]] to warn callers if they discard the actual amount of bytes read.
     *
     * @note This method does not block calling threads. This must be handled separately to allow for consecutive calls
     * to @c NoidFile::Read() by a single thread.
     *
     * @param destination The pointer to the destination data.
     * @param start_position The position in this file to start reading at.
     * @param size The amount of bytes to read.
     * @return The amount of bytes read.
     * @throws std::ios_base::failure if reading @p size bytes from the file fails.
     */
    [[nodiscard]] std::size_t Read(byte* destination, Position start_position, std::size_t size) override;

    /**
     * @brief Forces a write of all user-space buffered data to OS buffers.
     * @details This just flushes the user-space buffers to any buffered writes can be seen by readers. To ensure the
     * written data survives a reboot or a system crash, use @c NoidFile::Sync(), which synchronizes the kernel buffers
     * with the underlying storage device.
     *
     * @throws std::ios_base::failure if the flushing the user-space buffers fails.
     */
    void Flush() override;

    /**
     * @brief Transfer all modified data from kernel buffers to the underlying storage device.
     *
     * @details This function ensures that the file data survives a reboot or a system crash after this function
     * has returned.
     * Because of this guarantee, this method blocks until the underlying storage device reports that the  data transfer
     * completed successfully. Syncing a file must be done sparingly. An OS buffers file content aggressively,
     * and calling this function invalidates OS and disk caches and probably will degrade performance enormously if
     * called often.
     *
     * @note This function only synchronizes the minimum required file metadata to the storage device. Synchronizing
     * all file metadata is currently not supported.
     *
     * @throws std::ios_base::failure if synchronizing to the storage device fails.
     */
    void Sync() override;

    /**
     * @brief Acquires a unique lock for this file.
     * @details Tries to acquire the unique lock on this file. If another thread has the unique lock, this method
     * blocks until the unique lock can be acquired.
     *
     * @return The unique lock.
     * @throws std::system_error if any error occurs, including those from the underlying operating system.
     */
    UnixFileLock<IntentAwareMutex> UniqueLock() override;

    /**
     * @brief Acquires a unique lock for this file.
     * @details Tries to acquire the unique lock on this file. If another thread has the unique lock, this method
     * does not block and returns immediately.
     *
     * @return The unique lock, which is owned on success.
     * @throws std::system_error if any error occurs, including those from the underlying operating system.
     */
    UnixFileLock<IntentAwareMutex> TryUniqueLock() override;

    /**
     * @brief Acquires a shared lock for this file.
     * @details Tries to acquire a shared lock on this file. If another thread has acquired the unique lock or has
     * indicated its intent to unique the unique lock, this method blocks until the unique lock has been released and
     * a shared lock can be acquired.
     *
     * @return The shared lock.
     * @throws std::system_error if any error occurs, including those from the underlying operating system.
     */
    UnixSharedFileLock<IntentAwareMutex> SharedLock() override;

    /**
     * @brief Acquires a shared lock for this file.
     * @details Tries to acquire a shared lock on this file. If another thread has the unique lock, this method
     * does not block and returns immediately.
     *
     * @return The shared lock, which is owned on success.
     * @throws std::system_error if any error occurs, including those from the underlying operating system.
     */
    UnixSharedFileLock<IntentAwareMutex> TrySharedLock() override;

    /**
     * Destructs the @c UnixFile and closes the associated @c std::FILE. If that file is a temporary file, it is
     * also removed.
     */
    ~UnixFile() override;
};

}

#endif //NOID_SRC_BACKEND_VFS_UNIX_UNIXFILE_H_
