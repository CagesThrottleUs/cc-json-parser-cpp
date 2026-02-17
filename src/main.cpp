#include <exception>
#include <iostream>
#include <memory>
#include <span>
#include <sstream>
#include <utility>

#include "constants/constants.hpp"
#include "exceptions/file_operation_exception.hpp"
#include "exceptions/usage_exception.hpp"
#include "file_handler/utf8_file.hpp"
#include "utf8/checked.h"

namespace {

void handle_usage_error(const std::span<char*>& args);

void run_trial(const std::string& filename);

/** Encode one codepoint to UTF-8 for display. */
auto codepoint_to_utf8(char32_t codepoint) -> std::string;

}  // namespace

auto main(int argc, char** argv) -> int {
  std::span<char*> args(argv, argc);

  try {
    handle_usage_error(args);
    run_trial(args[1]);
    return std::to_underlying(constants::exit_codes::VALID_JSON);
  } catch (const exceptions::usage_exception& e) {
    std::cerr << e.what() << '\n';
    return std::to_underlying(constants::exit_codes::USAGE_ERROR);
  } catch (const exceptions::file_operation_exception& e) {
    std::cerr << e.what() << '\n';
    return std::to_underlying(constants::exit_codes::FILE_OPERATION_ERROR);
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return std::to_underlying(constants::exit_codes::UNKNOWN_ERROR);
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

void run_trial(const std::string& filename) {
  const constants::file_load_type load_type =
      file_handler::get_file_load_type(filename);
  const char* load_name = (load_type == constants::file_load_type::FullMemory)
                              ? "FullMemory"
                              : "MemoryMapped";

  std::unique_ptr<file_handler::Utf8File> file =
      file_handler::open_utf8_file(filename);

  std::cout << "--- file ---\n";
  std::cout << "  path:      " << filename << "\n";
  std::cout << "  load type: " << load_name << "\n";
  std::cout << "  size:      " << file->size() << " bytes\n\n";

  std::cout << "--- first 20 codepoints (hex + character) ---\n  ";
  std::string interpreted;
  constexpr int max_codepoints = 20;
  for (int count = 0; count < max_codepoints && !file->at_end(); ++count) {
    auto codepoint = file->next_codepoint();
    if (!codepoint) {
      break;
    }
    std::cout << " U+" << std::hex << static_cast<unsigned>(*codepoint)
              << std::dec << " '" << codepoint_to_utf8(*codepoint) << "'";
    utf8::append(static_cast<utf8::utfchar32_t>(*codepoint),
                 std::back_inserter(interpreted));
  }
  std::cout << "\n\n  interpreted: \"" << interpreted << "\"\n\n";

  file->reset();
  std::cout << "--- after reset ---\n";
  if (const auto peeked = file->peek_codepoint()) {
    std::cout << "  peek:  U+" << std::hex << static_cast<unsigned>(*peeked)
              << std::dec << " '" << codepoint_to_utf8(*peeked) << "'\n";
    file->advance();
  }
  std::cout << "  atEnd: " << (file->at_end() ? "yes" : "no") << "\n";
}

auto codepoint_to_utf8(char32_t codepoint) -> std::string {
  std::string out;
  utf8::append(static_cast<utf8::utfchar32_t>(codepoint),
               std::back_inserter(out));
  return out;
}

}  // namespace
