/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_VFS_NOIDFILE_H_
#define NOID_SRC_BACKEND_VFS_NOIDFILE_H_

#include <array>
#include <concepts>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <stdexcept>

#include "backend/Types.h"
#include "backend/DynamicArray.h"
#include "backend/concurrent/Concepts.h"

namespace fs = std::filesystem;

using namespace noid::backend::concurrent;

namespace noid::backend::vfs {

/**
 * @brief Abstraction for files from all noid VFS implementations.
 * @details The @c NoidFile::ReadContainer and @c NoidFile::WriteContainer overloads have been named like this to
 * prevent a @c static_cast to @c NoidFile<Lockable, SharedLockable> on implementing classes every time these functions
 * are to be called.
 *
 * @author houthacker
 */
template<Lockable Lockable, SharedLockable SharedLockable>
class NoidFile {
 protected:

    /**
     * @brief Atomically writes the @p length bytes from @p source to this file, starting at @p start_position.
     *
     * @note Implementations must ensure that this method does not block calling threads. This must be handled separately to
     * allow for consecutive calls to @c NoidFile::Write() by a single thread.
     *
     * @warning Using this method without concurrency measures in effect will almost certainly cause file corruption.
     *
     * @param source The pointer to the source data.
     * @param start_position The position in this file to start writing at.
     * @param size The amount of bytes to write.
     * @return The amount of bytes written.
     * @throws std::ios_base::failure if writing @p size bytes to the file fails.
     */
    virtual std::size_t Write(const byte* source, Position start_position, std::size_t size) =0;

    /**
     * @brief Reads @p size bytes from this file into @p destination, starting at @p start_position.
     *
     * @note Reading an arbitrary amount of bytes from a file is not atomic in the sense that all or nothing is read,
     * therefore this method is marked @c [[nodiscard]] to warn callers if they discard the actual amount of bytes read.
     *
     * @note Implementations must ensure that this method does not block calling threads. This must be handled separately to
     * allow for consecutive calls to @c NoidFile::Read() by a single thread.
     *
     * @param destination The pointer to the destination data.
     * @param start_position The position in this file to start reading at.
     * @param size The amount of bytes to read.
     * @return The amount of bytes read.
     * @throws std::ios_base::failure if reading @p size bytes from the file fails.
     */
    [[nodiscard]] virtual std::size_t Read(DynamicArray<byte>& container, DynamicArray<byte>::iterator container_pos,
                                           Position start_position, std::size_t size) =0;

    // for arrays
    [[nodiscard]] virtual std::size_t Read(byte* destination, Position start_position, std::size_t size) =0;

 public:
    virtual ~NoidFile() =default;

    /**
     * @brief Returns the file size in bytes.
     * @details This method does not block. This allows callers to decide themselves about the blocking scope.
     *
     * @return The file size.
     * @throw std::ios_base::failure if the file size could not be retrieved.
     */
    virtual std::uintmax_t Size() =0;

    /**
     * @brief Atomically writes the complete array contents to this file.
     *
     * @tparam N The length of the array.
     * @param source The array containing the source data.
     * @param start_position The file position to start writing at (inclusive).
     * @return The amount of bytes written.
     * @throws std::ios_base::failure if writing @p size bytes to the file fails.
     */
    template<std::size_t N>
    std::size_t WriteContainer(const std::array<byte, N>& source, Position start_position) {
      return this->Write(source.data(), start_position, N);
    }

    /**
     * @brief Atomically writes the indicated array data to this file.
     *
     * @tparam N The length of the array.
     * @param source The array containing the source data.
     * @param source_start The array index to start reading from (inclusive).
     * @param start_position The file position to start writing at (inclusive).
     * @param size The amount of bytes to write to this file.
     * @return The amount of bytes written.
     * @throws std::out_of_range if the given parameters would lead to out-of-bound reads from the source array.
     * @throws std::ios_base::failure if writing @p size bytes to the file fails.
     */
    template<std::size_t N>
    std::size_t WriteContainer(const std::array<byte, N>& source, Position source_start, Position start_position, std::size_t size) {
      if (source_start + size > N) {
        throw std::out_of_range("Write would lead to reading outside of source bounds.");
      }

      return this->Write(source.data() + source_start, start_position, size);
    }

    /**
     * @brief Atomically writes the complete vector contents to this file.
     *
     * @param source The vector containing the source data.
     * @param start_position The file position to start writing at (inclusive).
     * @return The amount of bytes written.
     * @throws std::ios_base::failure if writing the vectors' content to the file fails.
     */
    std::size_t WriteContainer(const DynamicArray<byte>& source, Position start_position) {
      return this->Write(source.data(), start_position, source.size());
    }

    /**
     * @brief Atomically writes the indicated vector data to this file.
     *
     * @param source The vector containing the source data.
     * @param source_start The vector index to start reading from (inclusive).
     * @param write_start The file position to start writing at (inclusive).
     * @param size The amount of bytes to write to this file.
     * @return The amount of bytes written.
     * @throws std::out_of_range if the given parameters would lead to out-of-bound reads from the source vector.
     * @throws std::ios_base::failure if writing @p size bytes to the file fails.
     */
    std::size_t WriteContainer(const DynamicArray<byte>& source, Position source_start, Position write_start, std::size_t size) {
      if (source_start + size > source.size()) {
        throw std::out_of_range("Write would lead to reading outside of vector bounds.");
      }

      return this->Write(source.data() + source_start, write_start, size);
    }

    /**
     * @brief Reads the requested amount of bytes into the destination array.
     *
     * @note Reading an arbitrary amount of bytes from a file is not atomic in the sense that all or nothing is read,
     * therefore this method is marked @c [[nodiscard]] to warn callers if they discard the actual amount of bytes read.
     *
     * @tparam N The array size in bytes.
     * @param destination The array to write the data into.
     * @param file_pos The file position to start reading at (inclusive).
     * @param size The amount of bytes to read from this file.
     * @return The actual amount of bytes read.
     * @throws std::ios_base::failure if reading from the file fails.
     */
    template<std::size_t N>
    [[nodiscard]] std::size_t ReadContainer(std::array<byte, N>& destination, Position file_pos) {
      return this->Read(destination.data(), file_pos, N);
    }

    /**
     * @brief Reads the requested amount of bytes into the destination array, starting at index @p destination_start.
     *
     * @note Reading an arbitrary amount of bytes from a file is not atomic in the sense that all or nothing is read,
     * therefore this method is marked @c [[nodiscard]] to warn callers if they discard the actual amount of bytes read.
     *
     * @param destination The array to write the data into.
     * @param destination_pos The array index to start writing at.
     * @param file_pos The file position to start reading at (inclusive).
     * @param size The amount of bytes to read from this file.
     * @return The actual amount of bytes read.
     * @throws std::out_of_range if reading the requested amount of bytes would cause a write outside of the array bounds.
     * @throws std::ios_base::failure if reading from the file fails.
     */
    template<std::size_t N>
    [[nodiscard]] std::size_t ReadContainer(std::array<byte, N>& destination, Position destination_pos, Position file_pos, std::size_t size) {
      if (destination_pos + size > N) {
        throw std::out_of_range("Read would lead to writing outside of destination bounds.");
      }

      return this->Read(destination.data() + destination_pos, file_pos, size);
    }

    /**
     * @brief Reads the requested amount of bytes into the destination vector.
     *
     * @note If reading @p size bytes into @p destination would cause extra memory allocation in the destination vector,
     * this method tries to reserve that space prior to reading from the file.
     *
     * @note Reading an arbitrary amount of bytes from a file is not atomic in the sense that all or nothing is read,
     * therefore this method is marked @c [[nodiscard]] to warn callers if they discard the actual amount of bytes read.
     *
     * @param destination The vector to write the data into.
     * @param file_pos The file position to start reading at (inclusive).
     * @param size The amount of bytes to read from this file.
     * @return The actual amount of bytes read.
     * @throws std::length_error if the new destination size exceeds @c destination.max_size().
     * @throws std::ios_base::failure if reading from the file fails.
     */
    [[nodiscard]]
    std::size_t ReadContainer(DynamicArray<byte>& destination, Position file_pos, std::size_t size) {
      return this->Read(destination, destination.begin(), file_pos, size);
    }

    /**
     * @brief Reads the requested amount of bytes into the destination vector.
     *
     * @note If reading @p size bytes into @p destination (starting at @p destination_start) would cause extra memory
     * allocation in the destination vector, this method tries to reserve that space prior to reading from the file.
     *
     * @note Reading an arbitrary amount of bytes from a file is not atomic in the sense that all or nothing is read,
     * therefore this method is marked @c [[nodiscard]] to warn callers if they discard the actual amount of bytes read.
     *
     * @param destination The vector to write the data into.
     * @param destination_pos The vector index to start writing at.
     * @param file_pos The file position to start reading at (inclusive).
     * @param size The amount of bytes to read from this file.
     * @return The actual amount of bytes read.
     * @throws std::length_error if the new destination size exceeds @c destination.max_size().
     * @throws std::ios_base::failure if reading from the file fails.
     */
    [[nodiscard]]
    std::size_t ReadContainer(DynamicArray<byte>& destination, Position destination_pos, Position file_pos, std::size_t size) {
      return this->Read(destination, destination.begin() + destination_pos, file_pos, size);
    }

    /**
     * @brief Forces a write of all user-space buffered data to OS buffers.
     * @details This just flushes the user-space buffers to any buffered writes can be seen by readers. To ensure the
     * written data survives a reboot or a system crash, use @c NoidFile::Sync(), which synchronizes the kernel buffers
     * with the underlying storage device.
     *
     * @throws std::ios_base::failure if the flushing the user-space buffers fails.
     */
    virtual void Flush() =0;

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
    virtual void Sync() =0;

    /**
     * @brief Acquires a unique lock for this file.
     * @details Tries to acquire the unique lock on this file. If another thread has the unique lock, this method
     * blocks until the unique lock can be acquired.
     *
     * @return The unique lock.
     * @throws std::system_error if any error occurs, including those from the underlying operating system.
     */
    virtual Lockable UniqueLock() =0;

    /**
     * @brief Acquires a unique lock for this file.
     * @details Tries to acquire the unique lock on this file. If another thread has the unique lock, this method
     * does not block and returns immediately.
     *
     * @return The unique lock, which is owned on success.
     * @throws std::system_error if any error occurs, including those from the underlying operating system.
     */
    virtual Lockable TryUniqueLock() =0;

    /**
     * @brief Acquires a shared lock for this file.
     * @details Tries to acquire a shared lock on this file. If another thread has acquired the unique lock or has
     * indicated its intent to unique the unique lock, this method blocks until the unique lock has been released and
     * a shared lock can be acquired.
     *
     * @return The shared lock.
     * @throws std::system_error if any error occurs, including those from the underlying operating system.
     */
    virtual SharedLockable SharedLock() =0;

    /**
     * @brief Acquires a shared lock for this file.
     * @details Tries to acquire a shared lock on this file. If another thread has the unique lock, this method
     * does not block and returns immediately.
     *
     * @return The shared lock, which is owned on success.
     * @throws std::system_error if any error occurs, including those from the underlying operating system.
     */
    virtual SharedLockable TrySharedLock() =0;
};

}

#endif //NOID_SRC_BACKEND_VFS_NOIDFILE_H_
