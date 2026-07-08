//
// Created by akul on 8/9/25.
//

#include <cppre/Parse.h>

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "cppre/AST.h"

namespace cppre::detail {

struct Parser {
    std::string_view pattern;
    size_t current_pos = 0;
    int group_id = 0;

    [[nodiscard]] auto rollback(int n = 1) {
        if (current_pos == 0)
            return;
        current_pos -= n;
    }

    [[nodiscard]] auto peek() const -> char {
        if (is_at_end())
            return '\0';
        return pattern.at(current_pos);
    }
    [[nodiscard]] auto peek_next() const -> char {
        if (current_pos + 1 >= pattern.size()) {
            return '\0';
        }
        return pattern.at(current_pos + 1);
    }
    auto advance() -> char { return pattern.at(current_pos++); }
    [[nodiscard]] auto is_at_end() const -> bool {
        return current_pos >= pattern.size();
    }

    [[nodiscard]] auto match(char c) -> bool {
        if (peek() == c) {
            advance();
            return true;
        }
        return false;
    }
};

static constexpr std::string_view kQuantifiers = "*+?";
static constexpr std::string_view kMetacharacters = ".|()[*+?^$";

enum EscapeType { Anchor, Char, CharClass };
static auto escape(char c, bool inside_char_class = false)
    -> std::pair<EscapeType, char> {
    switch (c) {
        case 'n':
            return {EscapeType::Char, '\n'};
        case 't':
            return {EscapeType::Char, '\t'};
        case 'r':
            return {EscapeType::Char, '\r'};
        case 'd':
        case 'w':
        case 's':
        case 'D':
        case 'W':
        case 'S':
            return {EscapeType::CharClass, c};
        case 'A':
        case 'Z':
        case 'B':
            return {EscapeType::Anchor, c};
        case 'b':
            return inside_char_class ? std::pair{EscapeType::Char, '\b'}
                                     : std::pair{EscapeType::Anchor, 'b'};

        default:
            return {EscapeType::Char, c};
    }
}

static auto parse_regex_impl(Parser& parser) -> ASTNodePtr;
static auto parse_alt_term(Parser& parser) -> ASTNodePtr;
static auto parse_concat_term(Parser& parser) -> ASTNodePtr;
static auto parse_string(Parser& parser) -> ASTNodePtr;
static auto parse_group(Parser& parser) -> ASTNodePtr;
static auto parse_char_class(Parser& parser) -> ASTNodePtr;

auto parse_regex(std::string_view pattern) -> ASTNodePtr {
    Parser parser{.pattern = pattern, .group_id = 1};

    auto regex = parse_regex_impl(parser);
    return std::make_unique<GroupNode>(std::move(regex), 0);
}

auto parse_regex_impl(Parser& parser) -> ASTNodePtr {
    ASTNodePtr left = parse_alt_term(parser);
    while (!parser.is_at_end() && parser.peek() == '|') {
        parser.advance();
        auto right = parse_alt_term(parser);
        left = std::make_unique<AlternationNode>(std::move(left),
                                                 std::move(right));
    }
    return left;
}

auto parse_alt_term(Parser& parser) -> ASTNodePtr {
    std::vector<ASTNodePtr> terms;
    while (!parser.is_at_end() && parser.peek() != '|' &&
           parser.peek() != ')') {
        auto term = parse_concat_term(parser);
        if (parser.peek() == '*' || parser.peek() == '+' ||
            parser.peek() == '?') {
            const char quant = parser.advance();
            QuantifierType qt = quant == '*'   ? QuantifierType::Star
                                : quant == '+' ? QuantifierType::Plus
                                               : QuantifierType::Optional;
            term = std::make_unique<RepNode>(std::move(term), qt);
        }
        terms.push_back(std::move(term));
    }

    if (terms.size() == 1) {
        return std::move(terms.front());
    } else {
        return std::make_unique<ConcatNode>(std::move(terms));
    }
}

auto parse_concat_term(Parser& parser) -> ASTNodePtr {
    switch (parser.peek()) {
        case '.':
            parser.advance();
            return std::make_unique<WildcardNode>();

        case '(':
            return parse_group(parser);

        case '[':
            return parse_char_class(parser);

        case ')':
            throw std::runtime_error(
                "Closing paranthesis '(' without corresponding open one");

        case '\\': {
            auto [esc_type, escaped] = escape(parser.peek_next());
            if (esc_type == EscapeType::CharClass) {
                parser.advance();
                return std::make_unique<ShortCharClass>(parser.advance());
            } else if (esc_type == EscapeType::Anchor) {
                parser.advance();
                return std::make_unique<AnchorNode>(
                    static_cast<AnchorNode::AnchorType>(parser.advance()));
            }
            return parse_string(parser);
        }

        case '^':
        case '$':
            return std::make_unique<AnchorNode>(
                static_cast<AnchorNode::AnchorType>(parser.advance()));

        default:
            return parse_string(parser);
    }
}

auto parse_string(Parser& parser) -> ASTNodePtr {
    const size_t old_start = parser.current_pos;
    size_t start = parser.current_pos;
    char quantified = '\0';
    std::string s;

    while (!parser.is_at_end() &&
           kMetacharacters.find(parser.peek()) == std::string::npos) {
        if (parser.peek() != '\\' &&
            kQuantifiers.find(parser.peek_next()) != std::string::npos) {
            break;
        }

        if (parser.peek() == '\\') {
            parser.advance();

            if (parser.is_at_end())
                break;

            auto [esc_type, escaped] = escape(parser.peek());

            if (esc_type != EscapeType::Char) {
                parser.rollback();
                break;
            }

            if (kQuantifiers.find(parser.peek_next()) != std::string::npos) {
                if (start == parser.current_pos - 1) {
                    parser.advance();
                    quantified = escaped;
                    break;
                }
                parser.rollback(1);
                break;
            }

            s += parser.pattern.substr(start, parser.current_pos - start - 1);
            s.push_back(escaped);
            parser.advance();
            start = parser.current_pos;
        } else {
            parser.advance();
        }
    }

    if ((start == old_start && start == parser.current_pos) ||
        quantified != '\0') {
        ASTNodePtr literal = std::make_unique<LiteralNode>(
            std::string(1, quantified ? quantified : parser.advance()));
        QuantifierType qt = static_cast<QuantifierType>(parser.advance());
        return std::make_unique<RepNode>(std::move(literal), qt);
    }

    s += parser.pattern.substr(start, parser.current_pos - start);
    return std::make_unique<LiteralNode>(s);
}

auto parse_group(Parser& parser) -> ASTNodePtr {
    parser.advance();
    int group_id;

    if (parser.peek() == '?') {
        parser.advance();
        if (parser.peek() != ':') {
            throw std::runtime_error(
                "Invalid group syntax (expected ':' after '?')");
        }
        parser.advance();
        group_id = -1;
    } else {
        group_id = parser.group_id++;
    }

    auto node = parse_regex_impl(parser);
    if (parser.is_at_end() || parser.peek() != ')') {
        throw std::runtime_error("Unmatched '(' in pattern");
    }
    parser.advance();
    return std::make_unique<GroupNode>(std::move(node), group_id);
}

namespace {
auto parse_char_class_char(Parser& parser) -> char {
    if (parser.is_at_end())
        return '\0';
    char c = parser.advance();
    if (c == '\\') {
        auto [esc_type, esc_char] = escape(parser.peek(), true);
        parser.advance();
        return esc_char;
    } else {
        return c;
    }
}
}  // namespace

auto parse_char_class(Parser& parser) -> ASTNodePtr {
    parser.advance();
    bool inverted = parser.match('^');

    auto node = std::make_unique<CharClassNode>(inverted);

    while (!parser.is_at_end() && parser.peek() != ']') {
        char rb = parse_char_class_char(parser);

        if (parser.peek() == '-') {
            parser.advance();
            if (parser.peek() == ']' || parser.is_at_end()) {
                node->in_class[rb] = true;
                node->in_class['-'] = true;
                break;
            }

            char rc = parse_char_class_char(parser);
            if (rb <= rc)
                std::fill(node->in_class.begin() + rb,
                          node->in_class.begin() + rc + 1, true);
        } else {
            node->in_class[rb] = true;
        }
    }

    if (parser.is_at_end() || parser.peek() != ']') {
        throw std::runtime_error("Unmatched '[' in pattern");
    }
    parser.advance();
    return node;
}

}  // namespace cppre::detail