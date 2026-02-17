#include <iostream>
#include <span>
#include <sstream>
#include <utility>

#include "constants/exit_codes.hpp"
#include "exceptions/usage_exception.hpp"

namespace {
void handle_usage_error(const std::span<char*>& args);
}  // namespace

auto main(int argc, char** argv) -> int {
  // span for args
  std::span<char*> args(argv, argc);

  // main logic
  try {
    handle_usage_error(args);
    std::cout << "Hello, from cc-json-parser-cpp!\n";
    return std::to_underlying(constants::exit_codes::VALID_JSON);
  } catch (const exceptions::usage_exception& e) {
    std::cerr << e.what() << '\n';
    return std::to_underlying(constants::exit_codes::USAGE_ERROR);
  }
}

namespace {

void handle_usage_error(const std::span<char*>& args) {
  if (args.size() < 2) {
    std::stringstream error_message;
    error_message << "Usage: " << args[0] << " <json_file>";
    throw exceptions::usage_exception(error_message.str());
  }
  if (args.size() > 2) {
    std::stringstream error_message;
    error_message << "Error: Only one JSON file can be provided";
    throw exceptions::usage_exception(error_message.str());
  }
}

}  // namespace
