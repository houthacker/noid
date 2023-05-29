/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_NOIDCONFIG_H_
#define NOID_SRC_BACKEND_NOIDCONFIG_H_

#include <cstdint>

namespace noid::backend {

enum class VFSType {

    /**
     * Indicates usage of Unix file operations and -locking.
     */
    Unix
};

class NoidConfig {
 private:
    NoidConfig() {
#if !defined(LINUX) && !defined(__linux__)
#error "Unsupported OS. Only linux is supported at this time."
#else
      this->vfs_type = VFSType::Unix;
#endif
    };

 public:

    /**
     * @return The singleton configuration instance.
     */
    static NoidConfig& Get() {
      static NoidConfig instance;
      return instance;
    }

    NoidConfig(NoidConfig const&) =delete;
    NoidConfig(NoidConfig &&) =delete;

    NoidConfig& operator=(const NoidConfig &) =delete;
    NoidConfig& operator=(NoidConfig &&) =delete;

    /**
     * The type of Virtual File System used at runtime.
     */
    VFSType vfs_type;

    /**
     * @brief Whether to flush kernel buffers to disk when closing a file.
     * @details If set, this option degrades performance by many orders of magnitude. But, it ensures that data
     * will survive a reboot or power failure. If this risk is acceptable (which is is in many cases), this option
     * should be set to @c false.
     */
    bool io_flush_kernel_buffers = false;

    /**
     * @brief The storage page size.
     */
    uint16_t vfs_page_size = 4096;
};

}
#endif //NOID_SRC_BACKEND_NOIDCONFIG_H_
