/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_UUID_H_
#define NOID_SRC_BACKEND_UUID_H_

#include <array>
#include <cstdint>
#include <string>

#include "Types.h"

namespace noid::backend {

/**
 * @brief The amount of bytes in a UUID.
 */
const uint8_t  UUID_BYTE_COUNT = 16;

/**
 * @brief A java-compatible UUID format.
 *
 * @details The @c UUID has a name UUID (v3) implementation which, for a given name,
 * produces a UUID that is identical to the result of @c java.util.UUID::nameUUIDFromBytes(byte[]).
 * The java-compatible UUID format differs from the regular UUID format in that, when creating a v3 UUID,
 * it does not use a predefined scope seed. Instead, it just creates the UUID based on the MD5 hash of the
 * given name, and sets the UUID metadata accordingly.
 *
 * @author houthacker
 */
class UUID {
 private:
    const std::array<byte, UUID_BYTE_COUNT> bytes;

    explicit UUID(std::array<byte, UUID_BYTE_COUNT> bytes);

 public:

    /**
     * @brief Recreates a previously created @c UUID from the given byte array.
     *
     * @param bytes The bytes from a previously created @c UUID.
     * @return The UUID (v3 or 4).
     */
    static UUID FromBytes(std::array<byte, UUID_BYTE_COUNT> bytes);

    /**
     * @brief Creates a new @c UUID based on the given @p name.
     *
     * @param name The name to hash.
     * @return A v3 @c UUID based on the given @p name.
     */
    static UUID NameUUID(std::string& name);

    /**
     * @return A pseudorandom v4 @c UUID.
     */
    static UUID RandomUUID();

    /**
     * @return A reference to the @c UUID byte array.
     */
    const std::array<byte, UUID_BYTE_COUNT>& Bytes();
};

}

#endif //NOID_SRC_BACKEND_UUID_H_
