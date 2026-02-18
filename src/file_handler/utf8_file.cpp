#include "utf8_file.hpp"

#include <boost/iostreams/device/mapped_file.hpp>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <span>
#include <string>
#include <string_view>

#include "../exceptions/file_operation_exception.hpp"
#include "utf8/checked.h"

namespace file_handler {

namespace detail {

namespace {

/**
 * Validates the file path.
 * @throws exceptions::file_operation_exception if file does not exist or is not
 * a regular file.
 */
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
  if (err_code) {
    throw exceptions::file_operation_exception("Cannot get file size: " +
                                               filename);
  }
}

}  // namespace

// --- Full in-memory implementation ---
class utf8_in_memory_file : public utf8_file {
 public:
  explicit utf8_in_memory_file(const std::string& filename)
      : file_name(filename) {
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
    
    // Resize buffer to hold the file content. 
    // std::vector guarantees contiguous memory storage.
    buffer.resize(stream_size);
    
    if (stream_size > 0U &&
        !stream.read(buffer.data(),
                     static_cast<std::streamsize>(stream_size))) {
      throw exceptions::file_operation_exception("Failed to read file: " +
                                                 filename);
    }
  }

  ~utf8_in_memory_file() override;
  utf8_in_memory_file(const utf8_in_memory_file&) = delete;
  auto operator=(const utf8_in_memory_file&) -> utf8_in_memory_file& = delete;
  utf8_in_memory_file(utf8_in_memory_file&&) = delete;
  auto operator=(utf8_in_memory_file&&) -> utf8_in_memory_file& = delete;

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

  void put_back(codepoint_type codepoint) override {
    if (pos == 0) {
      return;
    }
    try {
      auto iter = buffer.begin() + static_cast<std::ptrdiff_t>(pos);
      const codepoint_type prev = utf8::prior(iter, buffer.begin());
      if (prev != codepoint) {
        throw exceptions::file_operation_exception(
            "put_back: codepoint does not match byte at position");
      }
      pos = static_cast<std::size_t>(std::distance(buffer.begin(), iter));
    } catch (const utf8::not_enough_room&) {  // NOLINT(bugprone-empty-catch)
      /* Silently no-op: state unchanged. */
    } catch (const utf8::invalid_utf8&) {
      throw exceptions::file_operation_exception(
          "put_back: invalid UTF-8 at position");
    } catch (const utf8::exception& ex) {
      throw exceptions::file_operation_exception(std::string("put_back: ") +
                                                 ex.what());
    }
  }

  void revert() override {
    if (pos == 0) {
      return;
    }
    // utf8::prior decrements the iterator to the start of the previous codepoint
    auto iter = buffer.begin() + static_cast<std::ptrdiff_t>(pos);
    utf8::prior(iter, buffer.begin());
    pos = static_cast<std::size_t>(std::distance(buffer.begin(), iter));
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

  [[nodiscard]] auto data() const noexcept -> const char* override {
    return buffer.data();
  }

  [[nodiscard]] auto current_offset() const noexcept -> std::size_t override {
    return pos;
  }

 private:
  std::string file_name;
  /**
   * Raw buffer for file contents.
   * Using std::vector<char> instead of std::string because this is a raw byte
   * buffer that happens to be UTF-8, not necessarily a processed C-string.
   * std::vector avoids string semantics (like null-termination) and is the
   * standard container for contiguous memory buffers.
   */
  std::vector<char> buffer;
  std::size_t pos{0};
};

utf8_in_memory_file::~utf8_in_memory_file() = default;

// --- Memory-mapped implementation ---
class utf8_mmap_file : public utf8_file {
 public:
  explicit utf8_mmap_file(const std::string& filename) : file_name(filename) {
    validate_file_path(filename);
    mmap_file_src.open(filename);
    if (!mmap_file_src.is_open()) {
      throw exceptions::file_operation_exception("Failed to memory-map file: " +
                                                 filename);
    }
    content = std::span<const char>(
        std::string_view(mmap_file_src.data(), mmap_file_src.size()));
  }

  ~utf8_mmap_file() override;
  utf8_mmap_file(const utf8_mmap_file&) = delete;
  auto operator=(const utf8_mmap_file&) -> utf8_mmap_file& = delete;
  utf8_mmap_file(utf8_mmap_file&&) = delete;
  auto operator=(utf8_mmap_file&&) -> utf8_mmap_file& = delete;

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

  void put_back(codepoint_type codepoint) override {
    if (pos == 0) {
      return;
    }
    try {
      auto iter = content.begin() + static_cast<std::ptrdiff_t>(pos);
      const codepoint_type prev = utf8::prior(iter, content.begin());
      if (prev != codepoint) {
        throw exceptions::file_operation_exception(
            "put_back: codepoint does not match byte at position");
      }
      pos = static_cast<std::size_t>(std::distance(content.begin(), iter));
    } catch (const utf8::not_enough_room&) {  // NOLINT(bugprone-empty-catch)
      /* Silently no-op: state unchanged. */
    } catch (const utf8::invalid_utf8&) {
      throw exceptions::file_operation_exception(
          "put_back: invalid UTF-8 at position");
    } catch (const utf8::exception& ex) {
      throw exceptions::file_operation_exception(std::string("put_back: ") +
                                                 ex.what());
    }
  }

  void revert() override {
    if (pos == 0) {
      return;
    }
    auto iter = content.begin() + static_cast<std::ptrdiff_t>(pos);
    utf8::prior(iter, content.begin());
    pos = static_cast<std::size_t>(std::distance(content.begin(), iter));
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

  [[nodiscard]] auto data() const noexcept -> const char* override {
    return content.data();
  }

  [[nodiscard]] auto current_offset() const noexcept -> std::size_t override {
    return pos;
  }

 private:
  std::string file_name;
  boost::iostreams::mapped_file_source mmap_file_src;
  std::span<const char> content;
  std::size_t pos{0};
};

utf8_mmap_file::~utf8_mmap_file() = default;

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

auto open_utf8_file(const std::string& filename) -> std::unique_ptr<utf8_file> {
  const constants::file_load_type load_type = get_file_load_type(filename);
  if (load_type == constants::file_load_type::FullMemory) {
    return std::make_unique<detail::utf8_in_memory_file>(filename);
  }
  return std::make_unique<detail::utf8_mmap_file>(filename);
}

auto codepoint_to_utf8(char32_t codepoint) -> std::string {
  std::string out;
  utf8::append(static_cast<utf8::utfchar32_t>(codepoint),
               std::back_inserter(out));
  return out;
}

}  // namespace file_handler
