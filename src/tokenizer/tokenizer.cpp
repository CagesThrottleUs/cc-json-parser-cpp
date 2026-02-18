#include "tokenizer.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "../exceptions/tokenization_exception.hpp"

namespace tokenizer {

namespace detail {

/** JSON insignificant whitespace code points (U+0009, U+000A, U+000D, U+0020).
 */
constexpr char32_t JSON_TAB = 0x0009;
constexpr char32_t JSON_LINE_FEED = 0x000A;
constexpr char32_t JSON_CARRIAGE_RETURN = 0x000D;
constexpr char32_t JSON_SPACE = 0x0020;

/** Bundles line/column to avoid easily-swappable parameters. */
struct Position {
  std::size_t line = 0;
  std::size_t column = 0;
};

enum class State : int {  // NOLINT(performance-enum-size)
  START,
  // Literals
  IN_TRUE,
  IN_FALSE,
  IN_NULL,
  // Stops
  DEAD,
  COMPLETED,
  // Strings
  IN_STRING,
  STRING_ESCAPE_SEQUENCE,
  STRING_UNICODE_ESCAPE_SEQUENCE,
  HEX_ONE,
  HEX_TWO,
  HEX_THREE,
  // Numbers
  NUMBER_SIGN_START,
  ZERO_START,
  SIGNIFICANT_START,
  DECIMAL_START,
  DECIMAL_DIGIT,
  EXPONENT_START,
  EXPONENT_SIGN,
  EXPONENT_DIGIT,
};

namespace {

inline auto is_hex_digit(const std::string& utf8_codepoint) noexcept -> bool {
  return utf8_codepoint.size() == 1 &&
         ((utf8_codepoint[0] >= '0' && utf8_codepoint[0] <= '9') ||
          (utf8_codepoint[0] >= 'a' && utf8_codepoint[0] <= 'f') ||
          (utf8_codepoint[0] >= 'A' && utf8_codepoint[0] <= 'F'));
}

/** JSON disallows unescaped control characters U+0000..U+001F in strings. */
constexpr char32_t JSON_MAX_UNESPACED_CONTROL = 0x001F;

inline auto is_control_char(char32_t codepoint) noexcept -> bool {
  return codepoint <= JSON_MAX_UNESPACED_CONTROL;
}

/** Insignificant whitespace: tab, LF, CR, space. */
inline auto is_insignificant_whitespace(char32_t codepoint) noexcept -> bool {
  return codepoint == JSON_TAB || codepoint == JSON_LINE_FEED ||
         codepoint == JSON_CARRIAGE_RETURN || codepoint == JSON_SPACE;
}

inline auto format_error(const std::string& prefix, std::size_t line,
                         std::size_t col) -> std::string {
  return prefix + " at line " + std::to_string(line) + " and character " +
         std::to_string(col);
}

inline auto format_error(const std::string& prefix, std::size_t line,
                         std::size_t col, const std::string& suffix)
    -> std::string {
  return format_error(prefix, line, col) + suffix;
}

inline auto is_done(const State state) noexcept -> bool {
  return state == State::DEAD || state == State::COMPLETED;
}

inline auto is_digit(const std::string& utf8_codepoint) noexcept -> bool {
  return utf8_codepoint.size() == 1 &&
         (utf8_codepoint[0] == '-' ||
          (utf8_codepoint[0] >= '0' && utf8_codepoint[0] <= '9'));
}

inline auto determine_number_start(const std::string& utf8_codepoint) noexcept
    -> State {
  if (utf8_codepoint == "-") {
    return State::NUMBER_SIGN_START;
  }
  if (utf8_codepoint == "0") {
    return State::ZERO_START;
  }
  return State::SIGNIFICANT_START;
}

constexpr std::array<std::pair<std::string_view, std::pair<State, TokenType>>,
                     10>
    START_TRANSITIONS{{
        {"[", {State::COMPLETED, TokenType::TK_OPEN_BRACKET}},
        {"]", {State::COMPLETED, TokenType::TK_CLOSE_BRACKET}},
        {"{", {State::COMPLETED, TokenType::TK_OPEN_BRACE}},
        {"}", {State::COMPLETED, TokenType::TK_CLOSE_BRACE}},
        {":", {State::COMPLETED, TokenType::TK_COLON}},
        {",", {State::COMPLETED, TokenType::TK_COMMA}},
        {"t", {State::IN_TRUE, TokenType::END_OF_INPUT}},
        {"f", {State::IN_FALSE, TokenType::END_OF_INPUT}},
        {"n", {State::IN_NULL, TokenType::END_OF_INPUT}},
        {"\"", {State::IN_STRING, TokenType::END_OF_INPUT}},
    }};

inline auto handle_start(const std::string& utf8_codepoint) noexcept
    -> std::pair<State, TokenType> {
  const std::string_view key(utf8_codepoint);
  for (const auto& [arr_key, arr_val] : START_TRANSITIONS) {
    if (arr_key == key) {
      return arr_val;
    }
  }
  if (is_digit(utf8_codepoint)) {
    return {determine_number_start(utf8_codepoint), TokenType::END_OF_INPUT};
  }
  return {State::DEAD, TokenType::END_OF_INPUT};
}

inline auto handle_deterministic(const std::string& lexeme,
                                 const std::string& comparator) -> int {
  if (lexeme.size() > comparator.size()) {
    return -1;
  }
  if (comparator == lexeme) {
    return 1;
  }
  if (comparator.starts_with(lexeme)) {
    return 0;
  }
  return -1;
}

struct LiteralResult {
  State state;
  TokenType token_type;
  std::string error_message;
};

inline auto handle_literal(const std::string& lexeme,
                           const std::string& comparator,
                           TokenType completed_type,
                           const std::string& utf8_codepoint,
                           std::size_t line_num, std::size_t char_num)
    -> std::optional<LiteralResult> {
  const int ret = handle_deterministic(lexeme, comparator);
  if (ret == 1) {
    return LiteralResult{.state = State::COMPLETED,
                         .token_type = completed_type,
                         .error_message = ""};
  }
  if (ret == -1) {
    const char expected =
        comparator.at(std::min(lexeme.size(), comparator.size()) - 1);
    return LiteralResult{
        .state = State::DEAD,
        .token_type = TokenType::END_OF_INPUT,
        .error_message = format_error(
            "Unexpected character: " + utf8_codepoint, line_num, char_num,
            " expected character: " + std::string(1, expected) + " but got " +
                utf8_codepoint)};
  }
  return std::nullopt;
}

inline auto handle_literal_state(State current, const std::string& lexeme,
                                 const std::string& utf8_codepoint,
                                 std::size_t line_num, std::size_t char_num)
    -> std::optional<LiteralResult> {
  auto comparator = std::string{};
  auto completed_type = TokenType::END_OF_INPUT;
  switch (current) {
    case State::IN_TRUE:
      comparator = "true";
      completed_type = TokenType::TK_TRUE;
      break;
    case State::IN_FALSE:
      comparator = "false";
      completed_type = TokenType::TK_FALSE;
      break;
    case State::IN_NULL:
      comparator = "null";
      completed_type = TokenType::TK_NULL;
      break;
    default:
      return std::nullopt;
  }
  return handle_literal(lexeme, comparator, completed_type, utf8_codepoint,
                        line_num, char_num);
}

struct StringTransitionResult {
  State next_state;
  TokenType token_type;
  std::string error_message;
};

inline auto handle_string_state(State current, char32_t codepoint,
                                const std::string& utf8_codepoint,
                                std::size_t line, std::size_t col)
    -> StringTransitionResult {
  switch (current) {
    case State::IN_STRING:
      if (utf8_codepoint == "\"") {
        return {.next_state = State::COMPLETED,
                .token_type = TokenType::TK_STRING,
                .error_message = ""};
      }
      if (utf8_codepoint == "\\") {
        return {.next_state = State::STRING_ESCAPE_SEQUENCE,
                .token_type = TokenType::END_OF_INPUT,
                .error_message = ""};
      }
      if (is_control_char(codepoint)) {
        return {.next_state = State::DEAD,
                .token_type = TokenType::END_OF_INPUT,
                .error_message = format_error(
                    "Unescaped control character in string", line, col)};
      }
      return {.next_state = State::IN_STRING,
              .token_type = TokenType::END_OF_INPUT,
              .error_message = ""};

    case State::STRING_ESCAPE_SEQUENCE:
      if (utf8_codepoint == "\"" || utf8_codepoint == "\\" ||
          utf8_codepoint == "/" || utf8_codepoint == "b" ||
          utf8_codepoint == "f" || utf8_codepoint == "n" ||
          utf8_codepoint == "r" || utf8_codepoint == "t") {
        return {.next_state = State::IN_STRING,
                .token_type = TokenType::END_OF_INPUT,
                .error_message = ""};
      }
      if (utf8_codepoint == "u") {
        return {.next_state = State::STRING_UNICODE_ESCAPE_SEQUENCE,
                .token_type = TokenType::END_OF_INPUT,
                .error_message = ""};
      }
      return {.next_state = State::DEAD,
              .token_type = TokenType::END_OF_INPUT,
              .error_message =
                  format_error("Invalid escape sequence in string", line, col)};

    case State::STRING_UNICODE_ESCAPE_SEQUENCE:
    case State::HEX_ONE:
    case State::HEX_TWO:
    case State::HEX_THREE: {
      State next_state = State::DEAD;
      switch (current) {
        case State::STRING_UNICODE_ESCAPE_SEQUENCE:
          next_state = State::HEX_ONE;
          break;
        case State::HEX_ONE:
          next_state = State::HEX_TWO;
          break;
        case State::HEX_TWO:
          next_state = State::HEX_THREE;
          break;
        case State::HEX_THREE:
          next_state = State::IN_STRING;
          break;
        default:
          break;
      }
      if (is_hex_digit(utf8_codepoint)) {
        return {.next_state = next_state,
                .token_type = TokenType::END_OF_INPUT,
                .error_message = ""};
      }
      return {.next_state = State::DEAD,
              .token_type = TokenType::END_OF_INPUT,
              .error_message = format_error(
                  "Invalid hex digit in unicode escape sequence", line, col)};
    }
    default:
      return {.next_state = current,
              .token_type = TokenType::END_OF_INPUT,
              .error_message = ""};
  }
}

struct NumberTransitionResult {
  State next_state;
  TokenType token_type;
  std::string error_message;
  bool put_back_codepoint = false;
};

inline auto is_decimal_digit(const std::string& codepoint) noexcept -> bool {
  return codepoint.size() == 1 && codepoint[0] >= '0' && codepoint[0] <= '9';
}

inline auto number_ok(State next, bool put_back = false)
    -> NumberTransitionResult {
  return {.next_state = next,
          .token_type = TokenType::END_OF_INPUT,
          .error_message = "",
          .put_back_codepoint = put_back};
}

inline auto number_error(State next, const std::string& codepoint,
                         std::size_t line, std::size_t col,
                         const std::string& expected)
    -> NumberTransitionResult {
  return {.next_state = next,
          .token_type = TokenType::END_OF_INPUT,
          .error_message = format_error("Unexpected number: " + codepoint, line,
                                        col, expected),
          .put_back_codepoint = false};
}

auto handle_number_sign_start(const std::string& codepoint, std::size_t line,
                              std::size_t col) -> NumberTransitionResult {
  if (codepoint == "0") {
    return number_ok(State::ZERO_START);
  }
  if (codepoint.size() == 1 && codepoint[0] >= '1' && codepoint[0] <= '9') {
    return number_ok(State::SIGNIFICANT_START);
  }
  return number_error(State::DEAD, codepoint, line, col,
                      " expected number: [0-9]");
}

auto handle_zero_start(const std::string& codepoint) -> NumberTransitionResult {
  if (codepoint == ".") {
    return number_ok(State::DECIMAL_START);
  }
  if (codepoint == "e" || codepoint == "E") {
    return number_ok(State::EXPONENT_START);
  }
  return {.next_state = State::COMPLETED,
          .token_type = TokenType::TK_NUMBER,
          .error_message = "",
          .put_back_codepoint = true};
}

auto handle_significant_start(const std::string& codepoint)
    -> NumberTransitionResult {
  if (is_decimal_digit(codepoint)) {
    return number_ok(State::SIGNIFICANT_START);
  }
  if (codepoint == ".") {
    return number_ok(State::DECIMAL_START);
  }
  if (codepoint == "e" || codepoint == "E") {
    return number_ok(State::EXPONENT_START);
  }
  return {.next_state = State::COMPLETED,
          .token_type = TokenType::TK_NUMBER,
          .error_message = "",
          .put_back_codepoint = true};
}

auto handle_decimal_start(const std::string& codepoint, std::size_t line,
                          std::size_t col) -> NumberTransitionResult {
  if (is_decimal_digit(codepoint)) {
    return number_ok(State::DECIMAL_DIGIT);
  }
  return number_error(State::DEAD, codepoint, line, col,
                      " expected at least one decimal digit");
}

auto handle_decimal_digit(State /* current */, const std::string& codepoint,
                          std::size_t /* line */, std::size_t /* col */)
    -> NumberTransitionResult {
  if (is_decimal_digit(codepoint)) {
    return number_ok(State::DECIMAL_DIGIT);
  }
  if (codepoint == "e" || codepoint == "E") {
    return number_ok(State::EXPONENT_START);
  }
  return {.next_state = State::COMPLETED,
          .token_type = TokenType::TK_NUMBER,
          .error_message = "",
          .put_back_codepoint = true};
}

auto handle_exponent_start(const std::string& codepoint, std::size_t line,
                           std::size_t col) -> NumberTransitionResult {
  if (codepoint == "+" || codepoint == "-") {
    return number_ok(State::EXPONENT_SIGN);
  }
  if (is_decimal_digit(codepoint)) {
    return number_ok(State::EXPONENT_DIGIT);
  }
  return number_error(State::DEAD, codepoint, line, col,
                      " expected exponent sign or digit");
}

auto handle_exponent_sign(const std::string& codepoint, std::size_t line,
                          std::size_t col) -> NumberTransitionResult {
  if (is_decimal_digit(codepoint)) {
    return number_ok(State::EXPONENT_DIGIT);
  }
  return number_error(State::DEAD, codepoint, line, col,
                      " expected exponent digit [0-9]");
}

auto handle_exponent_digit(const std::string& codepoint)
    -> NumberTransitionResult {
  if (is_decimal_digit(codepoint)) {
    return number_ok(State::EXPONENT_DIGIT);
  }
  return {.next_state = State::COMPLETED,
          .token_type = TokenType::TK_NUMBER,
          .error_message = "",
          .put_back_codepoint = true};
}

inline auto handle_number_state(State current,
                                const std::string& utf8_codepoint,
                                std::size_t line, std::size_t col)
    -> NumberTransitionResult {
  switch (current) {
    case State::NUMBER_SIGN_START:
      return handle_number_sign_start(utf8_codepoint, line, col);
    case State::ZERO_START:
      return handle_zero_start(utf8_codepoint);
    case State::SIGNIFICANT_START:
      return handle_significant_start(utf8_codepoint);
    case State::DECIMAL_START:
      return handle_decimal_start(utf8_codepoint, line, col);
    case State::DECIMAL_DIGIT:
      return handle_decimal_digit(current, utf8_codepoint, line, col);
    case State::EXPONENT_START:
      return handle_exponent_start(utf8_codepoint, line, col);
    case State::EXPONENT_SIGN:
      return handle_exponent_sign(utf8_codepoint, line, col);
    case State::EXPONENT_DIGIT:
      return handle_exponent_digit(utf8_codepoint);
    default:
      return number_ok(current);
  }
}

void advance_past_dead_chars(file_handler::Utf8File& file, Position& pos) {
  auto state = State::DEAD;
  while (!file.at_end() && !is_done(state)) {
    auto codepoint = file.peek_codepoint();
    if (!codepoint) {
      break;
    }
    auto utf8_codepoint = file_handler::codepoint_to_utf8(*codepoint);
    auto [new_state, new_token_type] = handle_start(utf8_codepoint);
    if (new_state != State::DEAD) {
      break;
    }
    file.advance();
    if (utf8_codepoint == "\n") {
      pos.line++;
      pos.column = 1;
    } else {
      pos.column++;
    }
  }
}

struct StateStepResult {
  State next_state;
  TokenType token_type;
  std::string error_message;
  bool put_back = false;
};

auto process_state_step(State current_state, char32_t codepoint,
                        const std::string& utf8_codepoint,
                        const std::string& lexeme, std::size_t line_number,
                        std::size_t character_number) -> StateStepResult {
  switch (current_state) {
    case State::START: {
      auto [new_state, new_token_type] = handle_start(utf8_codepoint);
      std::string err;
      if (new_state == State::DEAD) {
        err = format_error("Unknown character: " + utf8_codepoint, line_number,
                           character_number);
      }
      return {.next_state = new_state,
              .token_type = new_token_type,
              .error_message = std::move(err)};
    }
    case State::IN_TRUE:
    case State::IN_FALSE:
    case State::IN_NULL: {
      auto res = handle_literal_state(current_state, lexeme, utf8_codepoint,
                                      line_number, character_number);
      if (!res) {
        return {.next_state = current_state,
                .token_type = TokenType::END_OF_INPUT,
                .error_message = {}};
      }
      return {.next_state = res->state,
              .token_type = res->token_type,
              .error_message = std::move(res->error_message)};
    }
    case State::IN_STRING:
    case State::STRING_ESCAPE_SEQUENCE:
    case State::STRING_UNICODE_ESCAPE_SEQUENCE:
    case State::HEX_ONE:
    case State::HEX_TWO:
    case State::HEX_THREE: {
      auto res = handle_string_state(current_state, codepoint, utf8_codepoint,
                                     line_number, character_number);
      return {.next_state = res.next_state,
              .token_type = res.token_type,
              .error_message = std::move(res.error_message)};
    }
    case State::NUMBER_SIGN_START:
    case State::ZERO_START:
    case State::SIGNIFICANT_START:
    case State::DECIMAL_START:
    case State::DECIMAL_DIGIT:
    case State::EXPONENT_START:
    case State::EXPONENT_SIGN:
    case State::EXPONENT_DIGIT: {
      auto num_res = handle_number_state(current_state, utf8_codepoint,
                                         line_number, character_number);
      return {.next_state = num_res.next_state,
              .token_type = num_res.token_type,
              .error_message = std::move(num_res.error_message),
              .put_back = num_res.put_back_codepoint};
    }
    default:
      return {.next_state = current_state,
              .token_type = TokenType::END_OF_INPUT,
              .error_message = {}};
  }
}

}  // namespace

}  // namespace detail

auto Tokenizer::next_token() -> Token {
  auto current_state = detail::State::START;
  std::string lexeme;
  TokenType token_type = TokenType::END_OF_INPUT;
  std::string error_message;
  while (!file->at_end() && !detail::is_done(current_state)) {
    auto codepoint = file->next_codepoint();
    if (!codepoint) {
      return Token{.type = token_type,
                   .lexeme = lexeme,
                   .line_number = line_number,
                   .character_number = character_number};
    }

    if (current_state == detail::State::START &&
        detail::is_insignificant_whitespace(*codepoint)) {
      if (*codepoint == detail::kJsonLineFeed ||
          *codepoint == detail::kJsonCarriageReturn) {
        line_number++;
        character_number = 1;
      } else {
        character_number++;
      }
      continue;
    }

    const auto utf8_codepoint = file_handler::codepoint_to_utf8(*codepoint);
    lexeme += utf8_codepoint;
    if (utf8_codepoint == "\n") {
      line_number++;
      character_number = 1;
    } else {
      character_number++;
    }

    auto result =
        detail::process_state_step(current_state, *codepoint, utf8_codepoint,
                                   lexeme, line_number, character_number);
    current_state = result.next_state;
    token_type = result.token_type;
    if (!result.error_message.empty()) {
      error_message = std::move(result.error_message);
    }
    if (result.put_back) {
      file->put_back(*codepoint);
      lexeme.pop_back();
    }
  }

  if (current_state == detail::State::DEAD) {
    detail::Position pos{.line = line_number, .column = character_number};
    detail::advance_past_dead_chars(*file, pos);
    line_number = pos.line;
    character_number = pos.column;
    throw exceptions::tokenization_exception(error_message);
  }

  return Token{.type = token_type,
               .lexeme = lexeme,
               .line_number = line_number,
               .character_number = character_number};
}

}  // namespace tokenizer