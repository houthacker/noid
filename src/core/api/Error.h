/**
 * Copyright 2023, noid authors. See LICENSE.md for licensing terms.
 */

#ifndef NOID_SRC_CORE_API_ERROR_H_
#define NOID_SRC_CORE_API_ERROR_H_

#include <string>
#include <iostream>
#ifdef __unix__
#include <cstring>
#else
#include <mutex>
static std::mutex strerr_mutex;
#endif

// Until a logging or observability system has been implemented, use these macros for logging.
#define NOID_LOG_TRACE(loc, msg) std::cout << "[TRACE] " << loc.file_name() << "(" << loc.line() << ":" << loc.column() << ") " << (msg) << std::endl
#define NOID_LOG_DEBUG(loc, msg) std::cout << "[DEBUG] " << loc.file_name() << "(" << loc.line() << ":" << loc.column() << ") " << (msg) << std::endl
#define NOID_LOG_INFO(loc, msg) std::cout << "[INFO] " << loc.file_name() << "(" << loc.line() << ":" << loc.column() << ") " << (msg) << std::endl
#define NOID_LOG_WARN(loc, msg) std::cerr << "[WARN] " << loc.file_name() << "(" << loc.line() << ":" << loc.column() << ") " << (msg) << std::endl
#define NOID_LOG_ERROR(loc, msg) std::cerr << "[ERROR] " << loc.file_name() << "(" << loc.line() << ":" << loc.column() << ") " << (msg) << std::endl
#define NOID_LOG_FATAL(loc, msg) std::cerr << "[FATAL] " << loc.file_name() << "(" << loc.line() << ":" << loc.column() << ") " << (msg) << std::endl

namespace noid::core::api {

static std::string GetErrorText(int errnum) {
#ifdef __unix__
  char buf[1024];
#if _POSIX_C_SOURCE >= 200112L && !defined(_GNU_SOURCE)
  // XSI-compliant version
  int result = strerror_r(errnum, buf, sizeof(buf));
  return {buf};
#else
  // GNU-specific version
  char* msg = strerror_r(errnum, buf, sizeof(buf));
  return {msg};
#endif
#else
#include <mutex>

  std::scoped_lock<std::mutex> lock(strerr_mutex);

  // Use strerror which is not MT-safe
  char* msg = strerror(errnum);
  return {msg};
#endif
}

}

#endif //NOID_SRC_CORE_API_ERROR_H_
