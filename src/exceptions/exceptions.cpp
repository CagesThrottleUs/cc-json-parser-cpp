#include "file_operation_exception.hpp"
#include "tokenization_exception.hpp"
#include "usage_exception.hpp"

namespace exceptions {

base_exception::~base_exception() = default;
file_operation_exception::~file_operation_exception() = default;
tokenization_exception::~tokenization_exception() = default;
usage_exception::~usage_exception() = default;

}  // namespace exceptions
