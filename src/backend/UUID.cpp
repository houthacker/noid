/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include "UUID.h"
#include "MD5.h"

#include <algorithm>
#include <limits>

namespace noid::backend {

UUID::UUID(std::array<byte, UUID_BYTE_COUNT> bytes) : bytes(bytes) {}

UUID UUID::FromBytes(std::array<byte, UUID_BYTE_COUNT> bytes) {
  return UUID(bytes);
}

UUID UUID::NameUUID(std::string &name) {
  auto md5 = MD5::Digest(name);
  auto md5_hash = md5.GetHash();

  std::array<byte, UUID_BYTE_COUNT> temp = {0};
  std::copy(temp.begin(), temp.end(), md5_hash.begin());

  temp[6] &= 0x0f;  // Clear version
  temp[6] |= 0x30;  // Set version to 3 (name UUID)
  temp[8] &= 0x3f;  // Clear UUID variant
  temp[8] |= 0x80;  // Set variant to IETF

  return UUID(temp);
}

UUID UUID::RandomUUID() {
  std::array<byte, UUID_BYTE_COUNT> temp = {0};
  for (byte & i : temp) {
    i = random() % std::numeric_limits<byte>::max();
  }

  temp[6] &= 0x0f;  // Clear version
  temp[6] |= 0x40;  // Set version to v4 (random UUID)
  temp[8] &= 0x3f;  // Clear UUID variant
  temp[8] |= 0x80;  // Set variant to IETF

  return UUID(temp);
}

const std::array<byte, UUID_BYTE_COUNT>& UUID::Bytes() {
  return this->bytes;
}

}