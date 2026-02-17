#ifndef EXCEPTIONS_TOKENIZATION_EXCEPTION_HPP
#define EXCEPTIONS_TOKENIZATION_EXCEPTION_HPP

#include <exception>
#include <string>
#include <utility>

namespace exceptions {

/**
 * Exception thrown when a tokenization error occurs.
 */
class tokenization_exception : public std::exception {
 public:
  explicit tokenization_exception(std::string message) noexcept
      : message_(std::move(message)) {}

  [[nodiscard]] auto what() const noexcept -> const char* override {
    return message_.c_str();
  }

 private:
  std::string message_;
};

}  // namespace exceptions
#endif  // EXCEPTIONS_TOKENIZATION_EXCEPTION_HPP