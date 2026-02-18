#ifndef EXCEPTIONS_USAGE_EXCEPTION_HPP
#define EXCEPTIONS_USAGE_EXCEPTION_HPP

#include <exception>
#include <string>

namespace exceptions {

/**
 * Exception thrown when a usage error occurs.
 */
class usage_exception : public std::exception {
 public:
  //  In this scenario, we are actually using the reference and copying -
  //  instead pass by value and moving the string
  //   usage_exception(const std::string& message)
  //       : message_(message) {}

  explicit usage_exception(std::string message)
      : message_(std::move(message)) {}

  ~usage_exception() override;
  usage_exception(const usage_exception&) = default;
  usage_exception(usage_exception&&) = default;
  auto operator=(const usage_exception&) -> usage_exception& = default;
  auto operator=(usage_exception&&) -> usage_exception& = default;

  [[nodiscard]] auto what() const noexcept -> const char* override {
    return message_.c_str();
  }

 private:
  std::string message_;
};

}  // namespace exceptions

#endif  // EXCEPTIONS_USAGE_EXCEPTION_HPP
