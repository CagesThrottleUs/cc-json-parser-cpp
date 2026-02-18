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

/**
 * @brief A lightweight, non-owning view over the command-line arguments.
 *
 * This class wraps the raw C-style `argv` array and `argc` count into a
 * C++ range-compatible interface. It allows utilizing C++20 ranges and
 * spans with command-line arguments without deep copying.
 */
class argv_view {
 public:
  /**
   * @brief Constructs a view from the raw argument vector and count.
   * @param data Pointer to the array of C-strings (argv).
   * @param n Number of arguments (argc).
   */
  explicit argv_view(char** data, std::size_t n) : data_(data), size_(n) {}

  /** @brief Returns an iterator to the first argument. */
  [[nodiscard]] auto begin() const -> char** { return data_; }

  /** @brief Returns an iterator one past the last argument. */
  [[nodiscard]] auto end() const -> char** {
    return std::next(data_, static_cast<std::ptrdiff_t>(size_));
  }

  /** @brief Returns the number of arguments. */
  [[nodiscard]] auto size() const -> std::size_t { return size_; }

  /** @brief Returns the underlying raw pointer. */
  [[nodiscard]] auto data() const -> char** { return data_; }

 private:
  char** data_;       ///< Pointer to the beginning of the argv array.
  std::size_t size_;  ///< Number of elements in the array.
};

}  // namespace detail

// Standard permits specializing enable_borrowed_range for program-defined types
// ([namespace.std]). NOLINTNEXTLINE(cert-dcl58-cpp)
namespace std::ranges {

/**
 * @brief Opt-in to the borrowed_range concept for argv_view.
 *
 * This specialization informs the ranges library that iterators obtained from
 * an rvalue `argv_view` remain valid after the view is destroyed. This is
 * safe because `argv_view` does not own the underlying memory (the OS does).
 */
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
