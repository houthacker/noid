/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#include "Noid.h"

namespace noid::core::api {

std::unique_ptr<Noid> Noid::Configure() {
  return std::unique_ptr<Noid>(new Noid());
}

}