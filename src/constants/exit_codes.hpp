#ifndef CONSTANTS_EXIT_CODES_HPP
#define CONSTANTS_EXIT_CODES_HPP

namespace constants {

/**
 * Exit codes for the program.
 */
enum class exit_codes : int {  // NOLINT(performance-enum-size)
  VALID_JSON = 0,
  INVALID_JSON = 1,
  USAGE_ERROR = 2,
};

}  // namespace constants

#endif  // CONSTANTS_EXIT_CODES_HPP