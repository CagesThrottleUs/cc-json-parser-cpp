#include <exception>
#include <iostream>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "constants/constants.hpp"
#include "exceptions/file_operation_exception.hpp"
#include "exceptions/tokenization_exception.hpp"
#include "exceptions/usage_exception.hpp"
#include "file_handler/utf8_file.hpp"
#include "tokenizer/tokenizer.hpp"

namespace {

constexpr std::size_t kTableSeparatorWidth = 60;

void handle_usage_error(const std::span<char*>& args);
auto token_type_label(tokenizer::TokenType type) -> const char*;
void print_tokens_table(const std::vector<tokenizer::Token>& tokens);
void print_errors_table(const std::vector<std::string>& errors);
auto tokenize_collecting_tokens_and_errors(tokenizer::Tokenizer& tok)
    -> std::pair<std::vector<tokenizer::Token>, std::optional<std::string>>;

}  // namespace

auto main(int argc, char** argv) -> int {
  std::span<char*> args(argv, argc);

  try {
    handle_usage_error(args);
    const std::string filename(args[1]);
    auto file = file_handler::open_utf8_file(filename);
    tokenizer::Tokenizer tok(std::move(file));
    auto [tokens, error] = tokenize_collecting_tokens_and_errors(tok);
    print_tokens_table(tokens);
    if (error) {
      print_errors_table({*error});
      return std::to_underlying(constants::exit_codes::INVALID_JSON);
    }
    return std::to_underlying(constants::exit_codes::VALID_JSON);
  } catch (const exceptions::usage_exception& e) {
    std::cerr << e.what() << '\n';
    return std::to_underlying(constants::exit_codes::USAGE_ERROR);
  } catch (const exceptions::file_operation_exception& e) {
    std::cerr << e.what() << '\n';
    return std::to_underlying(constants::exit_codes::FILE_OPERATION_ERROR);
  } catch (const exceptions::tokenization_exception& e) {
    std::cerr << e.what() << '\n';
    return std::to_underlying(constants::exit_codes::INVALID_JSON);
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return std::to_underlying(constants::exit_codes::UNKNOWN_ERROR);
  } catch (...) {
    std::cerr << "Unknown exception occurred\n";
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

auto token_type_label(tokenizer::TokenType type) -> const char* {
  switch (type) {
    case tokenizer::TK_STRING:
      return "TK_STRING";
    case tokenizer::TK_NUMBER:
      return "TK_NUMBER";
    case tokenizer::TK_TRUE:
      return "TK_TRUE";
    case tokenizer::TK_FALSE:
      return "TK_FALSE";
    case tokenizer::TK_NULL:
      return "TK_NULL";
    case tokenizer::TK_OPEN_BRACE:
      return "TK_OPEN_BRACE";
    case tokenizer::TK_CLOSE_BRACE:
      return "TK_CLOSE_BRACE";
    case tokenizer::TK_OPEN_BRACKET:
      return "TK_OPEN_BRACKET";
    case tokenizer::TK_CLOSE_BRACKET:
      return "TK_CLOSE_BRACKET";
    case tokenizer::TK_COLON:
      return "TK_COLON";
    case tokenizer::TK_COMMA:
      return "TK_COMMA";
    case tokenizer::END_OF_INPUT:
      return "END_OF_INPUT";
    default:
      return "?";
  }
}

void print_tokens_table(const std::vector<tokenizer::Token>& tokens) {
  std::cout << "Tokens (" << tokens.size() << "):\n";
  if (tokens.empty()) {
    std::cout << "  (none)\n";
    return;
  }
  const char* sep = " | ";
  std::cout << "#" << sep << "Type" << sep << "Lexeme" << sep << "Line"
            << sep << "Column\n";
  std::cout << std::string(kTableSeparatorWidth, '-') << '\n';
  for (std::size_t i = 0; i < tokens.size(); ++i) {
    const auto& token = tokens.at(i);
    std::cout << (i + 1) << sep << token_type_label(token.type) << sep
              << token.lexeme << sep << token.line_number << sep
              << token.character_number << '\n';
  }
}

void print_errors_table(const std::vector<std::string>& errors) {
  std::cerr << "Tokenization errors (" << errors.size() << "):\n";
  if (errors.empty()) {
    return;
  }
  const char* sep = " | ";
  std::cerr << "#" << sep << "Message\n"
            << std::string(kTableSeparatorWidth, '-') << '\n';
  for (std::size_t i = 0; i < errors.size(); ++i) {
    std::cerr << (i + 1) << sep << errors.at(i) << '\n';
  }
}

auto tokenize_collecting_tokens_and_errors(tokenizer::Tokenizer& tok)
    -> std::pair<std::vector<tokenizer::Token>, std::optional<std::string>> {
  std::vector<tokenizer::Token> tokens;
  try {
    for (;;) {
      tokenizer::Token token = tok.consume();
      tokens.push_back(token);
      if (token.type == tokenizer::END_OF_INPUT) {
        break;
      }
    }
    return {std::move(tokens), std::nullopt};
  } catch (const exceptions::tokenization_exception& e) {
    return {std::move(tokens), std::optional<std::string>(e.what())};
  }
}

}  // namespace
