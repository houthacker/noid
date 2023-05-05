#include <filesystem>

#include "Pager.h"
#include "exceptions/CannotOpenFile.h"

namespace noid::backend {

Pager::Pager(std::fstream file) : file(std::move(file)) {}

std::shared_ptr<Pager> Pager::Open(const fs::path& path) {
  try {
    auto stream = std::fstream(path, std::ios_base::binary | std::ios_base::in | std::ios_base::out);

    // Throw exceptions on read- and write errors, including failing to open the file.
    stream.exceptions(std::fstream::badbit);
    return std::shared_ptr<Pager>(new Pager(std::move(stream)));
  } catch (const std::fstream::failure& error) {
    throw CannotOpenFile(error.what());
  }
}

}