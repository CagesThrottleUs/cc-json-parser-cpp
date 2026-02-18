#ifndef EXCEPTIONS_FILE_OPERATION_EXCEPTION_HPP
#define EXCEPTIONS_FILE_OPERATION_EXCEPTION_HPP

#include "base_exception.hpp"

namespace exceptions {

/**
 * Exception thrown when a file operation fails.
 */
class file_operation_exception : public base_exception {
 public:
  using base_exception::base_exception;  // Inherit constructors

  ~file_operation_exception() override;
  file_operation_exception(const file_operation_exception&) = default;
  file_operation_exception(file_operation_exception&&) = default;
  auto operator=(const file_operation_exception&) -> file_operation_exception& = default;
  auto operator=(file_operation_exception&&) -> file_operation_exception& = default;
};

}  // namespace exceptions

#endif  // EXCEPTIONS_FILE_OPERATION_EXCEPTION_HPP
