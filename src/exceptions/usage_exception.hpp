#ifndef EXCEPTIONS_USAGE_EXCEPTION_HPP
#define EXCEPTIONS_USAGE_EXCEPTION_HPP

#include <exception>
#include <string>

namespace exceptions {

class usage_exception : public std::exception {
 public:
  //  In this scenario, we are actually using the reference and copying -
  //  instead pass by value and moving the string
  //   usage_exception(const std::string& message)
  //       : message_(message) {}

  explicit usage_exception(std::string message)
      : message_(std::move(message)) {}

  [[nodiscard]] auto what() const noexcept -> const char* override {
    return message_.c_str();
  }

 private:
  std::string message_;
};

}  // namespace exceptions

#endif  // EXCEPTIONS_USAGE_EXCEPTION_HPP