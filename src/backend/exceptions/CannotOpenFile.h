/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_E_CANNOTOPENFILE_H_
#define NOID_SRC_BACKEND_E_CANNOTOPENFILE_H_

#include <stdexcept>
#include <string>

namespace noid::backend {

class CannotOpenFile : public std::runtime_error {
 public:
    explicit CannotOpenFile(const char* message) : std::runtime_error(message) {}
    explicit CannotOpenFile(const std::string& message) noexcept : std::runtime_error(message) {}

    ~CannotOpenFile() override = default;
};
}
#endif //NOID_SRC_BACKEND_E_CANNOTOPENFILE_H_
