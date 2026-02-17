#ifndef CC_JSON_PARSER_FILE_HANDLER_UTF8_FILE_HPP
#define CC_JSON_PARSER_FILE_HANDLER_UTF8_FILE_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

#include "../constants/constants.hpp"

namespace file_handler {

/** Max file size for memory mapping (100 MiB). Below this we load fully. */
constexpr std::size_t MAX_MAPPED_FILE_SIZE = 100ULL * 1024ULL * 1024ULL;

/**
 * Abstract UTF-8 file: sequence of code points with peek, advance, next.
 */
class Utf8File {
 public:
  using codepoint_type = char32_t;

  virtual ~Utf8File() = default;
  Utf8File(const Utf8File&) = delete;
  auto operator=(const Utf8File&) -> Utf8File& = delete;
  Utf8File(Utf8File&&) = delete;
  auto operator=(Utf8File&&) -> Utf8File& = delete;

 protected:
  Utf8File() = default;

 public:
  virtual auto next_codepoint() -> std::optional<codepoint_type> = 0;
  [[nodiscard]] virtual auto peek_codepoint() const
      -> std::optional<codepoint_type> = 0;
  virtual auto advance() -> bool = 0;
  virtual void reset() = 0;
  [[nodiscard]] virtual auto good() const -> bool = 0;
  [[nodiscard]] virtual auto name() const -> std::string = 0;
  [[nodiscard]] virtual auto size() const noexcept -> std::size_t = 0;
  [[nodiscard]] virtual auto at_end() const noexcept -> bool = 0;
};

/**
 * Returns which load type would be used for the given file (by size).
 * @throws exceptions::file_operation_exception if file cannot be stat'd.
 */
auto get_file_load_type(const std::string& filename)
    -> constants::file_load_type;

/**
 * Opens the file with the appropriate implementation: full memory if
 * size < MAX_MAPPED_FILE_SIZE, else memory-mapped.
 */
auto open_utf8_file(const std::string& filename) -> std::unique_ptr<Utf8File>;

}  // namespace file_handler

#endif
