#ifndef PARSER_PARSER_HPP
#define PARSER_PARSER_HPP

#include <vector>

#include "../tokenizer/tokenizer.hpp"

namespace parser {

enum class ParserState {  // NOLINT(performance-enum-size)
  EXPECT_VALUE,
  EXPECT_OBJECT_KEY,
  EXPECT_OBJECT_COLON,
  EXPECT_OBJECT_VALUE,
  EXPECT_OBJECT_COMMA,
  EXPECT_ARRAY_VALUE,
  EXPECT_ARRAY_COMMA
};

class Parser {
 public:
  explicit Parser(tokenizer::Tokenizer&& tok) noexcept;
  void parse();
  [[nodiscard]] auto ok() const noexcept -> bool;

 private:
  // State Handlers
  void handle_value(tokenizer::TokenType type);
  void handle_object_key(tokenizer::TokenType type);
  void handle_object_colon(tokenizer::TokenType type);
  void handle_object_comma(tokenizer::TokenType type);
  void handle_array_value(tokenizer::TokenType type);
  void handle_array_comma(tokenizer::TokenType type);

  [[nodiscard]] auto peek_type() -> tokenizer::TokenType;

  tokenizer::Tokenizer tok_;
  bool is_valid_ = true;
  std::vector<ParserState> stack_;
};

}  // namespace parser

#endif  // PARSER_PARSER_HPP
