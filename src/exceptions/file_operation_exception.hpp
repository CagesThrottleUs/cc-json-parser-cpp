#ifndef EXCEPTIONS_FILE_OPERATION_EXCEPTION_HPP
#define EXCEPTIONS_FILE_OPERATION_EXCEPTION_HPP

#include <exception>
#include <string>

namespace exceptions {

/**
 * Exception thrown when a file operation fails.
 */
class file_operation_exception : public std::exception {
 public:
  explicit file_operation_exception(std::string message)
      : message_(std::move(message)) {}

  [[nodiscard]] auto what() const noexcept -> const char* override {
    return message_.c_str();
  }

 private:
  std::string message_;
};

}  // namespace exceptions

#endif  // EXCEPTIONS_FILE_OPERATION_EXCEPTION_HPP