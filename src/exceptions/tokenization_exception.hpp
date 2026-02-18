#ifndef EXCEPTIONS_TOKENIZATION_EXCEPTION_HPP
#define EXCEPTIONS_TOKENIZATION_EXCEPTION_HPP

#include "base_exception.hpp"

namespace exceptions {

/**
 * Exception thrown when a tokenization error occurs.
 */
class tokenization_exception : public base_exception {
 public:
  using base_exception::base_exception;  // Inherit constructors

  ~tokenization_exception() override;
  tokenization_exception(const tokenization_exception&) = default;
  tokenization_exception(tokenization_exception&&) = default;
  auto operator=(const tokenization_exception&) -> tokenization_exception& = default;
  auto operator=(tokenization_exception&&) -> tokenization_exception& = default;
};

}  // namespace exceptions
#endif  // EXCEPTIONS_TOKENIZATION_EXCEPTION_HPP
