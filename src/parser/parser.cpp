#include "parser.hpp"

namespace parser {

Parser::Parser(tokenizer::Tokenizer&& tok) noexcept : tok_(std::move(tok)) {}

auto Parser::ok() const noexcept -> bool { return is_valid_; }

auto Parser::peek_type() -> tokenizer::TokenType {
  auto type = tok_.peek().type;
  if (type == tokenizer::TK_ERROR) {
    is_valid_ = false;
  }
  return type;
}

void Parser::parse() {
  stack_.push_back(ParserState::EXPECT_VALUE);

  while (!stack_.empty() && is_valid_) {
    const auto current_state = stack_.back();
    stack_.pop_back();
    const auto type = peek_type();

    if (type == tokenizer::TK_ERROR) {
      is_valid_ = false;
      continue;
    }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-default"
    switch (current_state) {
      case ParserState::EXPECT_VALUE:
        handle_value(type);
        break;
      case ParserState::EXPECT_OBJECT_KEY:
        handle_object_key(type);
        break;
      case ParserState::EXPECT_OBJECT_COLON:
        handle_object_colon(type);
        break;
      case ParserState::EXPECT_OBJECT_VALUE:
        stack_.push_back(ParserState::EXPECT_OBJECT_COMMA);
        handle_value(type);
        break;
      case ParserState::EXPECT_OBJECT_COMMA:
        handle_object_comma(type);
        break;
      case ParserState::EXPECT_ARRAY_VALUE:
        handle_array_value(type);
        break;
      case ParserState::EXPECT_ARRAY_COMMA:
        handle_array_comma(type);
        break;
    }
#pragma clang diagnostic pop
  }

  if (is_valid_ && peek_type() != tokenizer::END_OF_INPUT) {
    is_valid_ = false;
  }
}

void Parser::handle_value(tokenizer::TokenType type) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
  switch (type) {
    case tokenizer::TK_STRING:
    case tokenizer::TK_NUMBER:
    case tokenizer::TK_TRUE:
    case tokenizer::TK_FALSE:
    case tokenizer::TK_NULL:
      tok_.consume();
      break;
    case tokenizer::TK_OPEN_BRACE:
      tok_.consume();
      stack_.push_back(ParserState::EXPECT_OBJECT_KEY);
      break;
    case tokenizer::TK_OPEN_BRACKET:
      tok_.consume();
      stack_.push_back(ParserState::EXPECT_ARRAY_VALUE);
      break;
    default:
      is_valid_ = false;
      break;
  }
#pragma clang diagnostic pop
}

void Parser::handle_object_key(tokenizer::TokenType type) {
  if (type == tokenizer::TK_CLOSE_BRACE) {
    tok_.consume();
  } else if (type == tokenizer::TK_STRING) {
    tok_.consume();
    stack_.push_back(ParserState::EXPECT_OBJECT_COLON);
  } else {
    is_valid_ = false;
  }
}

void Parser::handle_object_colon(tokenizer::TokenType type) {
  if (type == tokenizer::TK_COLON) {
    tok_.consume();
    stack_.push_back(ParserState::EXPECT_OBJECT_VALUE);
  } else {
    is_valid_ = false;
  }
}

void Parser::handle_object_comma(tokenizer::TokenType type) {
  if (type == tokenizer::TK_CLOSE_BRACE) {
    tok_.consume();
  } else if (type == tokenizer::TK_COMMA) {
    tok_.consume();
    if (peek_type() == tokenizer::TK_STRING) {
      stack_.push_back(ParserState::EXPECT_OBJECT_KEY);
    } else {
      is_valid_ = false;  // No trailing comma
    }
  } else {
    is_valid_ = false;
  }
}

void Parser::handle_array_value(tokenizer::TokenType type) {
  if (type == tokenizer::TK_CLOSE_BRACKET) {
    tok_.consume();
  } else {
    stack_.push_back(ParserState::EXPECT_ARRAY_COMMA);
    handle_value(type);
  }
}

void Parser::handle_array_comma(tokenizer::TokenType type) {
  if (type == tokenizer::TK_CLOSE_BRACKET) {
    tok_.consume();
  } else if (type == tokenizer::TK_COMMA) {
    tok_.consume();
    auto next = peek_type();
    if (next == tokenizer::TK_CLOSE_BRACKET ||
        next == tokenizer::END_OF_INPUT) {
      is_valid_ = false;  // No trailing comma
    } else {
      stack_.push_back(ParserState::EXPECT_ARRAY_VALUE);
    }
  } else {
    is_valid_ = false;
  }
}

}  // namespace parser
