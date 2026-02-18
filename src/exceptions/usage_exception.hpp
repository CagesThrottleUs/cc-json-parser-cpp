#ifndef EXCEPTIONS_USAGE_EXCEPTION_HPP
#define EXCEPTIONS_USAGE_EXCEPTION_HPP

#include "base_exception.hpp"

namespace exceptions {

/**
 * Exception thrown when a usage error occurs.
 */
class usage_exception : public base_exception {
 public:
  using base_exception::base_exception;  // Inherit constructors

  ~usage_exception() override;
  usage_exception(const usage_exception&) = default;
  usage_exception(usage_exception&&) = default;
  auto operator=(const usage_exception&) -> usage_exception& = default;
  auto operator=(usage_exception&&) -> usage_exception& = default;
};

}  // namespace exceptions

#endif  // EXCEPTIONS_USAGE_EXCEPTION_HPP
