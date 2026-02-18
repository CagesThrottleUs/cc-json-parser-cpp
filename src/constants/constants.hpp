#ifndef CONSTANTS_EXIT_CODES_HPP
#define CONSTANTS_EXIT_CODES_HPP

namespace constants {

/**
 * Exit codes for the program.
 */
// Int is standard for exit codes, and we don't
// use arrays of this enum where size would matter for cache locality.
enum class exit_codes : int {  // NOLINT(performance-enum-size)

  VALID_JSON = 0,
  INVALID_JSON = 1,
  USAGE_ERROR = 2,
  UNKNOWN_ERROR = 3,
  FILE_OPERATION_ERROR = 4,
};

/** How the file is backed: full buffer in memory or memory-mapped.
 * Single usage, not stored in containers.
 * Using int avoids unnecessary casting and alignment padding issues.
 */
enum class file_load_type {  // NOLINT(performance-enum-size)
  FullMemory,
  MemoryMapped,
};

}  // namespace constants

#endif  // CONSTANTS_EXIT_CODES_HPP
