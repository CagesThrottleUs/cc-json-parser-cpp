#include "utf8_file.hpp"

#include <boost/iostreams/device/mapped_file.hpp>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <span>
#include <string>

#include "../exceptions/file_operation_exception.hpp"
#include "utf8/checked.h"

namespace file_handler {

namespace detail {

namespace {

void validate_file_path(const std::string& filename) {
  namespace fs = std::filesystem;
  std::error_code err_code;
  const fs::path file_path(filename);
  if (!fs::exists(file_path, err_code) || err_code) {
    throw exceptions::file_operation_exception("File does not exist: " +
                                               filename);
  }
  if (!fs::is_regular_file(file_path, err_code) || err_code) {
    throw exceptions::file_operation_exception("Not a regular file: " +
                                               filename);
  }
  const auto file_size = fs::file_size(file_path, err_code);
  if (err_code) {
    throw exceptions::file_operation_exception("Cannot get file size: " +
                                               filename);
  }
}

}  // namespace

// --- Full in-memory implementation ---
class Utf8FileInMemory : public Utf8File {
 public:
  explicit Utf8FileInMemory(const std::string& filename) : file_name(filename) {
    std::ifstream stream(filename, std::ios::binary);
    if (!stream) {
      throw exceptions::file_operation_exception("Cannot open file: " +
                                                 filename);
    }
    stream.seekg(0, std::ios::end);
    const auto stream_size = static_cast<std::size_t>(stream.tellg());
    if (stream_size > MAX_MAPPED_FILE_SIZE) {
      throw exceptions::file_operation_exception(
          "File too large for full load (max " +
          std::to_string(MAX_MAPPED_FILE_SIZE) + "): " + filename);
    }
    stream.seekg(0);
    buffer.resize(stream_size);
    if (stream_size > 0U &&
        !stream.read(buffer.data(),
                     static_cast<std::streamsize>(stream_size))) {
      throw exceptions::file_operation_exception("Failed to read file: " +
                                                 filename);
    }
  }

  auto next_codepoint() -> std::optional<codepoint_type> override {
    if (pos >= buffer.size()) {
      return std::nullopt;
    }
    auto iter = buffer.begin() + static_cast<std::ptrdiff_t>(pos);
    const codepoint_type codepoint = utf8::next(iter, buffer.end());
    pos = static_cast<std::size_t>(std::distance(buffer.begin(), iter));
    return codepoint;
  }

  [[nodiscard]] auto peek_codepoint() const
      -> std::optional<codepoint_type> override {
    if (pos >= buffer.size()) {
      return std::nullopt;
    }
    auto iter = buffer.begin() + static_cast<std::ptrdiff_t>(pos);
    return utf8::peek_next(iter, buffer.end());
  }

  auto advance() -> bool override {
    if (pos >= buffer.size()) {
      return false;
    }
    auto iter = buffer.begin() + static_cast<std::ptrdiff_t>(pos);
    utf8::next(iter, buffer.end());
    pos = static_cast<std::size_t>(std::distance(buffer.begin(), iter));
    return true;
  }

  void reset() override { pos = 0; }

  [[nodiscard]] auto good() const -> bool override { return true; }

  [[nodiscard]] auto name() const -> std::string override { return file_name; }

  [[nodiscard]] auto size() const noexcept -> std::size_t override {
    return buffer.size();
  }

  [[nodiscard]] auto at_end() const noexcept -> bool override {
    return pos >= buffer.size();
  }

 private:
  std::string file_name;
  std::string buffer;
  std::size_t pos{0};
};

// --- Memory-mapped implementation ---
class Utf8MappedFile : public Utf8File {
 public:
  explicit Utf8MappedFile(const std::string& filename) : file_name(filename) {
    validate_file_path(filename);
    mmap_file_src.open(filename);
    if (!mmap_file_src.is_open()) {
      throw exceptions::file_operation_exception("Failed to memory-map file: " +
                                                 filename);
    }
    content = std::span<const char>(mmap_file_src.data(), mmap_file_src.size());
  }

  auto next_codepoint() -> std::optional<codepoint_type> override {
    if (pos >= content.size()) {
      return std::nullopt;
    }
    auto iter = content.begin() + static_cast<std::ptrdiff_t>(pos);
    const codepoint_type codepoint = utf8::next(iter, content.end());
    pos = static_cast<std::size_t>(std::distance(content.begin(), iter));
    return codepoint;
  }

  [[nodiscard]] auto peek_codepoint() const
      -> std::optional<codepoint_type> override {
    if (pos >= content.size()) {
      return std::nullopt;
    }
    auto iter = content.begin() + static_cast<std::ptrdiff_t>(pos);
    return utf8::peek_next(iter, content.end());
  }

  auto advance() -> bool override {
    if (pos >= content.size()) {
      return false;
    }
    auto iter = content.begin() + static_cast<std::ptrdiff_t>(pos);
    utf8::next(iter, content.end());
    pos = static_cast<std::size_t>(std::distance(content.begin(), iter));
    return true;
  }

  void reset() override { pos = 0; }

  [[nodiscard]] auto good() const -> bool override {
    return mmap_file_src.is_open();
  }

  [[nodiscard]] auto name() const -> std::string override { return file_name; }

  [[nodiscard]] auto size() const noexcept -> std::size_t override {
    return content.size();
  }

  [[nodiscard]] auto at_end() const noexcept -> bool override {
    return pos >= content.size();
  }

 private:
  std::string file_name;
  boost::iostreams::mapped_file_source mmap_file_src;
  std::span<const char> content;
  std::size_t pos{0};
};

}  // namespace detail

auto get_file_load_type(const std::string& filename)
    -> constants::file_load_type {
  namespace fs = std::filesystem;
  std::error_code err_code;
  const fs::path file_path(filename);
  if (!fs::exists(file_path, err_code) || err_code) {
    throw exceptions::file_operation_exception("File does not exist: " +
                                               filename);
  }
  if (!fs::is_regular_file(file_path, err_code) || err_code) {
    throw exceptions::file_operation_exception("Not a regular file: " +
                                               filename);
  }
  const auto file_size = fs::file_size(file_path, err_code);
  if (err_code) {
    throw exceptions::file_operation_exception("Cannot get file size: " +
                                               filename);
  }
  return file_size < MAX_MAPPED_FILE_SIZE
             ? constants::file_load_type::FullMemory
             : constants::file_load_type::MemoryMapped;
}

auto open_utf8_file(const std::string& filename) -> std::unique_ptr<Utf8File> {
  const constants::file_load_type load_type = get_file_load_type(filename);
  if (load_type == constants::file_load_type::FullMemory) {
    return std::make_unique<detail::Utf8FileInMemory>(filename);
  }
  return std::make_unique<detail::Utf8MappedFile>(filename);
}

}  // namespace file_handler
