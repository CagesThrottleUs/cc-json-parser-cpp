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
class utf8_file {
 public:
  using codepoint_type = char32_t;

  virtual ~utf8_file() = default;
  utf8_file(const utf8_file&) = delete;
  auto operator=(const utf8_file&) -> utf8_file& = delete;
  utf8_file(utf8_file&&) = delete;
  auto operator=(utf8_file&&) -> utf8_file& = delete;

 protected:
  utf8_file() = default;

 public:
  /**
   * Returns the next codepoint from the file.
   * @return The next codepoint, or std::nullopt if at end of file.
   */
  virtual auto next_codepoint() -> std::optional<codepoint_type> = 0;

  /**
   * Returns the next codepoint from the file without advancing the file
   * pointer.
   * @return The next codepoint, or std::nullopt if at end of file.
   */
  [[nodiscard]] virtual auto peek_codepoint() const
      -> std::optional<codepoint_type> = 0;

  /**
   * Advances the file pointer by one codepoint.
   * @return True if successful, false if at end of file.
   */
  virtual auto advance() -> bool = 0;

  /**
   * Puts a codepoint back so the next read returns it.
   * @param codepoint The codepoint to put back.
   */
  virtual void put_back(codepoint_type codepoint) = 0;

  /**
   * Resets the file pointer to the beginning of the file.
   */
  virtual void reset() = 0;

  /**
   * Returns true if the file is in a good state.
   */
  [[nodiscard]] virtual auto good() const -> bool = 0;

  /**
   * Returns the name of the file.
   */
  [[nodiscard]] virtual auto name() const -> std::string = 0;

  /**
   * Returns the size of the file.
   */
  [[nodiscard]] virtual auto size() const noexcept -> std::size_t = 0;

  /**
   * Returns true if the file pointer is at the end of the file.
   */
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
auto open_utf8_file(const std::string& filename) -> std::unique_ptr<utf8_file>;

auto codepoint_to_utf8(char32_t codepoint) -> std::string;

}  // namespace file_handler

#endif
