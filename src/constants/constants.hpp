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
  UNKNOWN_ERROR = 3,
  FILE_OPERATION_ERROR = 4,
};

/** How the file is backed: full buffer in memory or memory-mapped. */
// NOLINTNEXTLINE(performance-enum-size)
enum class file_load_type { FullMemory, MemoryMapped };

}  // namespace constants

#endif  // CONSTANTS_EXIT_CODES_HPP