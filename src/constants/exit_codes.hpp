namespace constants {

enum class exit_codes : int {  // NOLINT(performance-enum-size)
  VALID_JSON = 0,
  INVALID_JSON = 1,
  USAGE_ERROR = 2,
};

}