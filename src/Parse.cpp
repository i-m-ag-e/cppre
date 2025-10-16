//
// Created by akul on 8/9/25.
//

#include <cppre/Parse.h>

namespace cppre::detail {

struct Parser {
    std::string pattern;
    size_t current_pos = 0;
    int group_id = 0;

    [[nodiscard]] auto peek() const -> char {
        if (is_at_end())
            return '\0';
        return pattern.at(current_pos);
    }
    [[nodiscard]] auto peek_next() const -> char {
        if (is_at_end() || current_pos + 1 >= pattern.size()) {
            return '\0';
        }
        return pattern.at(current_pos + 1);
    }
    auto advance() -> char { return pattern.at(current_pos++); }
    [[nodiscard]] auto is_at_end() const -> bool {
        return current_pos >= pattern.size();
    }
};

static constexpr std::string_view kQuantifiers = "*+?";
static constexpr std::string_view kMetacharacters = ".|()[]*+?";

static auto parse_regex_impl(Parser& parser) -> ASTNodePtr;
static auto parse_alt_term(Parser& parser) -> ASTNodePtr;
static auto parse_concat_term(Parser& parser) -> ASTNodePtr;
static auto parse_string(Parser& parser) -> ASTNodePtr;
static auto parse_group(Parser& parser) -> ASTNodePtr;

auto parse_regex(const std::string& pattern) -> ASTNodePtr {
    Parser parser{.pattern = pattern};

    return parse_regex_impl(parser);
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

        default:
            return parse_string(parser);
    }
}

auto parse_string(Parser& parser) -> ASTNodePtr {
    const size_t start = parser.current_pos;
    while (!parser.is_at_end() &&
           kMetacharacters.find(parser.peek()) == std::string::npos) {
        if (kQuantifiers.find(parser.peek_next()) != std::string::npos) {
            break;
        }
        parser.advance();
    }

    if (start == parser.current_pos) {
        ASTNodePtr literal =
            std::make_unique<LiteralNode>(std::string(1, parser.advance()));
        QuantifierType qt = parser.peek() == '*'   ? QuantifierType::Star
                            : parser.peek() == '+' ? QuantifierType::Plus
                                                   : QuantifierType::Optional;
        parser.advance();
        return std::make_unique<RepNode>(std::move(literal), qt);
    }
    return std::make_unique<LiteralNode>(
        parser.pattern.substr(start, parser.current_pos - start));
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

}  // namespace cppre::detail
