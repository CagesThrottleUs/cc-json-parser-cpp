#ifndef EXCEPTIONS_BASE_EXCEPTION_HPP
#define EXCEPTIONS_BASE_EXCEPTION_HPP

#include <exception>
#include <string>
#include <utility>

namespace exceptions {

/**
 * @brief Base class for exceptions with allocation-safe message handling.
 *
 * This class addresses the issue where allocating a std::string for an
 * exception message during an OOM (Out Of Memory) situation could trigger
 * a secondary OOM exception and program termination.
 *
 * It stores messages in one of two ways:
 * 1. A std::string for dynamic messages (when memory is available).
 * 2. A const char* for static literals or fallback messages (zero allocation).
 */
class base_exception : public std::exception {
 public:
  /**
   * @brief Construct with a dynamic string message.
   *
   * @note If allocation of the internal string fails (e.g., OOM),
   * this constructor catches the std::bad_alloc and falls back to
   * a static "Memory allocation failed during exception" message.
   */
  explicit base_exception(std::string message) noexcept {
    try {
      str_message_ = std::move(message);
    } catch (...) {
      // Fallback if move/allocation fails
      raw_message_ = "Memory allocation failed during exception creation";
    }
  }

  /**
   * @brief Construct with a static string literal.
   *
   * @param message A string literal that has static storage duration.
   * Do NOT pass temporary C-strings here.
   */
  explicit base_exception(const char* message) noexcept
      : raw_message_(message) {}

  ~base_exception() override;
  base_exception(const base_exception&) = default;
  base_exception(base_exception&&) = default;
  auto operator=(const base_exception&) -> base_exception& = default;
  auto operator=(base_exception&&) -> base_exception& = default;

  [[nodiscard]] auto what() const noexcept -> const char* override {
    return str_message_.empty() ? raw_message_ : str_message_.c_str();
  }

 private:
  // Primary storage for dynamic messages.
  std::string str_message_;
  // Storage for static string literals or fallback messages.
  const char* raw_message_ = "Unknown error";
};

}  // namespace exceptions

#endif  // EXCEPTIONS_BASE_EXCEPTION_HPP
