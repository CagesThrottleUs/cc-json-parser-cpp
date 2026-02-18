#ifndef TOKENIZER_TOKENIZER_HPP
#define TOKENIZER_TOKENIZER_HPP

#include <memory>

#include "../file_handler/utf8_file.hpp"

namespace tokenizer {

/**
 * Types of tokens that can be returned by the tokenizer.
 */
enum TokenType : int {  // NOLINT(performance-enum-size)
  TK_STRING,
  TK_NUMBER,
  TK_TRUE,
  TK_FALSE,
  TK_NULL,
  TK_OPEN_BRACE,
  TK_CLOSE_BRACE,
  TK_OPEN_BRACKET,
  TK_CLOSE_BRACKET,
  TK_COLON,
  TK_COMMA,
  END_OF_INPUT,
  /** Emitted on tokenization error when parsing in best-effort mode. */
  TK_ERROR
};

/**
 * A token is a unit of input that is recognized by the tokenizer.
 */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
struct Token {
  std::string lexeme;
  std::size_t line_number;
  std::size_t character_number;
  TokenType type;
};

class Tokenizer {
 private:
  std::unique_ptr<file_handler::utf8_file> file;
  Token current;
  std::size_t line_number = 0;
  std::size_t character_number = 0;
  bool has_current = false;

 public:
  /**
   * Constructs a tokenizer for the given file.
   * @param _file The file to tokenize.
   */
  explicit Tokenizer(std::unique_ptr<file_handler::utf8_file>&& _file) noexcept
      : file(std::move(_file)), current{} {}

  /**
   * Peeks the next token without consuming it.
   * @return The next token.
   */
  auto peek() -> const Token& {
    if (!has_current) {
      current = next_token();
      has_current = true;
    }
    return current;
  }

  /**
   * Consumes the next token.
   * @return The next token.
   */
  auto consume() -> Token {
    if (!has_current) {
      current = next_token();
    }
    has_current = false;
    return current;
  }

 private:
  /**
   * Produces the next token from the file.
   * @return The next token.
   */
  [[nodiscard]] auto next_token() -> Token;
};
#pragma clang diagnostic pop

}  // namespace tokenizer

#endif  // TOKENIZER_TOKENIZER_HPP
