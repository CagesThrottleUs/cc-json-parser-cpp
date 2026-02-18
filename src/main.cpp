#include <iostream>
#include <iterator>
#include <span>

#include "constants/constants.hpp"
#include "exceptions/file_operation_exception.hpp"
#include "exceptions/usage_exception.hpp"
#include "file_handler/utf8_file.hpp"
#include "parser/parser.hpp"
#include "tokenizer/tokenizer.hpp"

namespace detail {

/** Non-owning view of argv for std::span; pairs pointer and size in one object.
 */
class argv_view {
 public:
  explicit argv_view(char** data, std::size_t n) : data_(data), size_(n) {}
  [[nodiscard]] auto begin() const -> char** { return data_; }
  [[nodiscard]] auto end() const -> char** {
    return std::next(data_, static_cast<std::ptrdiff_t>(size_));
  }
  [[nodiscard]] auto size() const -> std::size_t { return size_; }
  [[nodiscard]] auto data() const -> char** { return data_; }

 private:
  char** data_;
  std::size_t size_;
};

}  // namespace detail

// Standard permits specializing enable_borrowed_range for program-defined types
// ([namespace.std]). NOLINTNEXTLINE(cert-dcl58-cpp)
namespace std::ranges {
template <>
inline constexpr bool enable_borrowed_range<detail::argv_view> = true;
}  // namespace std::ranges

namespace {

void handle_usage(const std::span<char*>& args) {
  if (args.size() < 2) {
    throw exceptions::usage_exception(std::string("Usage: ") + args[0] +
                                      " <json_file>");
  }
  if (args.size() > 2) {
    throw exceptions::usage_exception(
        "Error: Only one JSON file can be provided");
  }
}

}  // namespace

auto main(int argc, char** argv) -> int {
  detail::argv_view view(argv, static_cast<std::size_t>(argc));
  std::span<char*> args(view);
  try {
    handle_usage(args);
    auto file = file_handler::open_utf8_file(std::string(args[1]));
    tokenizer::Tokenizer tok(std::move(file));
    parser::Parser json_parser(std::move(tok));
    json_parser.parse();

    if (!json_parser.ok()) {
      return std::to_underlying(constants::exit_codes::INVALID_JSON);
    }
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
  } catch (...) {
    std::cerr << "Unknown exception occurred\n";
    return std::to_underlying(constants::exit_codes::UNKNOWN_ERROR);
  }
}
