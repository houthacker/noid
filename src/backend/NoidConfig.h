/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_BACKEND_NOIDCONFIG_H_
#define NOID_SRC_BACKEND_NOIDCONFIG_H_

namespace noid::backend {

enum class VFSType {

    /**
     * Indicates usage of Unix file operations and -locking.
     */
    Unix
};

class NoidConfig {
 private:
    NoidConfig() : io_flush_kernel_buffers(false) {
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
     * Whether to flush kernel buffers whe closing a file.
     */
    bool io_flush_kernel_buffers;
};

}
#endif //NOID_SRC_BACKEND_NOIDCONFIG_H_
