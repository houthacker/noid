/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_CORE_API_NOID_H_
#define NOID_SRC_CORE_API_NOID_H_

#include <memory>
#include <string>

namespace noid::core::api {

/**
 * @brief Connections to a noid database all get an instance of this class.
 *
 * @author houthacker
 */
class Noid {
 private:
    Noid() =default;
 public:
    static std::unique_ptr<Noid> Configure();
};

}

#endif //NOID_SRC_CORE_API_NOID_H_
