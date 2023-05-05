/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_MD5_H_
#define NOID_SRC_BACKEND_MD5_H_

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "Types.h"

namespace noid::backend {

/**
 * @brief The amount of bytes in an MD5 hash.
 */
const uint8_t MD5_BYTE_COUNT = 16;

/**
 * Contains the state of the MD5 hash process.
 */
typedef struct MD5Context {

    /**
     * The size of the input in bytes.
     */
    uint64_t size;

    /**
     * The current accumulation of the hash.
     */
    std::array<uint32_t, 4> buffer;

    /**
     * The input to be used in the next step.
     */
    std::array<byte, 64> input;

    /**
     * When finished processing, this contains
     * the MD5 hash.
     */
    std::array<byte, MD5_BYTE_COUNT> digest;

} MD5Context;

/**
 * @brief Implementation ofthe MD5 hashing algorithm.
 * @see https://en.wikipedia.org/wiki/MD5
 *
 * The MD5 implementation used here was originally copied from https://github.com/Zunawe/md5-c.
 *
 * @authors Zunawe, houthacker
 */
class MD5 {
 private:
    const std::array<byte, MD5_BYTE_COUNT> hash;

    explicit MD5(std::array<byte, MD5_BYTE_COUNT> digest);

    static void Step(std::array<uint32_t, 4>& buffer, const std::array<uint32_t, MD5_BYTE_COUNT>& input);
    static void Update(MD5Context &ctx, const byte input[MD5_BYTE_COUNT], size_t input_len);
    static void Finalize(MD5Context &ctx);

 public:

/**
 * @brief Creates an MD5-hash based on the given string.
 *
 * @details This is a shorthand method for @code MD5::Digest((uint8_t*)value.c_str(), value.length())
 *
 * @param value The string to digest.
 * @return A new MD5 instance containing the hash.
 */
    static MD5 Digest(std::string& value);

/**
 * @brief Creates an MD5-hash based on the \p value of the given \p length.
 *
 * @param value The bytes to digest.
 * @param length The amount of bytes to digest.
 *
 * @return A new MD5 instance containing the hash.
 */
    static MD5 Digest(const std::vector<byte>& value, size_t length);

/**
 * @brief Creates an MD5-hash based on the given stream.
 * @details Note that this method doesn't reset the read position before hashing.
 *
 * @param stream The stream to create the MD5 hash from.
 * @return A new MD5 instance containing the hash.
 */
    static MD5 Digest(std::istream& stream);

/**
 * @brief Returns the MD5 hash, which is always MD5_BYTE_COUNT in length.
 *
 * @return The MD5 hash.
 */
    std::array<byte, MD5_BYTE_COUNT> GetHash();

/**
 * @brief Returns the hex-formatted MD5, without '-' signs.
 *
 * @return The MD5 hash.
 */
    std::string ToHexString();
};

}

#endif //NOID_SRC_BACKEND_MD5_H_
