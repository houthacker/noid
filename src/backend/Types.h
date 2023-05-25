/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_TYPES_H_
#define NOID_SRC_BACKEND_TYPES_H_

#include <cstdint>

namespace noid::backend {

using byte = unsigned char;
using PageNumber = std::uint32_t;
using Position = int64_t;

constexpr int8_t C_API_ERROR = -1;
}

#endif //NOID_SRC_BACKEND_TYPES_H_
