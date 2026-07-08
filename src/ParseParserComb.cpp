//
// Created by akul on 8/9/25.
//

#include <cppre/Parse.h>
#include <parser_comb/ParserComb.h>

#include <functional>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "cppre/AST.h"
#include "parser_comb/combinators/Alt.h"
#include "parser_comb/combinators/FMap.h"
#include "parser_comb/combinators/Sequence.h"
#include "parser_comb/primitives/Char.h"

namespace cppre::detail {

static constexpr std::string_view kMetacharacters = ".|()[*+?^$";

constexpr auto is_scc_char(char c) -> bool {
    return c == 'd' || c == 'w' || c == 's' || c == 'D' || c == 'W' || c == 'S';
}

constexpr auto is_anchor_escape(char c) -> bool {
    return c == 'A' || c == 'Z' || c == 'b' || c == 'B';
}

constexpr auto is_not_literal_char(char c) -> bool {
    return is_scc_char(c) || is_anchor_escape(c);
};

constexpr auto escape_map(char c) -> char {
    switch (c) {
        case 'n':
            return '\n';
        case 't':
            return '\t';
        case 'b':
            return '\b';
        case 'r':
            return '\r';
        default:
            return c;
    }
}

constexpr auto is_meta(char c) -> bool {
    return kMetacharacters.find(c) != std::string::npos;
};

constexpr auto quantifierP =
    comb::alt(comb::map_to_value(comb::charP('?'), QuantifierType::Optional),
              comb::map_to_value(comb::charP('+'), QuantifierType::Plus),
              comb::map_to_value(comb::charP('*'), QuantifierType::Star));

// A character in  literal string
// - either a '\\' followed by a character that does NOT make a literal char
// - or a character that is not '\\' or a meta character
constexpr auto literalCharP = comb::alt(
    comb::fmap(&escape_map,
               comb::right(comb::charP('\\'),
                           comb::satisfy(std::not_fn(is_not_literal_char)))),
    comb::satisfy([](char c) { return c != '\\' && !is_meta(c); }));

// We need to parse abcd+ as abc(d+) and not (abcd)+
// For that, we need to define a "safe character" (one that is not a quantified
// character, but a part of a normal literal string)
//
// So this is basically a `literalCharP` NOT followed by a quantifier
constexpr auto safeCharLiteralP =
    comb::left(literalCharP, comb::not_ahead(quantifierP));

// This parses a single quantified character (like d in abcd+)
constexpr auto singleCharLiteralP = comb::fmap(
    [](char c) -> ASTNodePtr {
        return std::make_unique<LiteralNode>(std::string(1, c));
    },
    literalCharP);

// this is a string of "literal characters" not followed by a quantifier
// for example, abc in abcd+ (d+ wll be picked up by singleCharLiteralP)
constexpr auto literalStringP = comb::fmap(
    [](std::vector<char>&& chars) -> ASTNodePtr {
        std::string lit = std::string(chars.begin(), chars.end());
        return std::make_unique<LiteralNode>(lit);
    },
    comb::one_or_more(safeCharLiteralP));

// the full literal parser
// a string like abcd+ will be parsed by this parser twice, first picking up
// "abc", then "d"
constexpr auto literalP = comb::alt(literalStringP, singleCharLiteralP);

// \d, \D, \w, etc.
constexpr auto shortCharClassP = comb::fmap(
    [](char c) -> ASTNodePtr { return std::make_unique<ShortCharClass>(c); },
    comb::right(comb::charP('\\'), comb::satisfy(&is_scc_char)));

constexpr auto anchorP = comb::fmap(
    [](char c) -> ASTNodePtr {
        return std::make_unique<AnchorNode>(
            static_cast<AnchorNode::AnchorType>(c));
    },
    comb::alt(
        comb::charP('^'), comb::charP('$'),
        comb::right(comb::charP('\\'), comb::satisfy(&is_anchor_escape))));

// .
constexpr auto wildcardP = comb::fmap(
    [](char) -> ASTNodePtr { return std::make_unique<WildcardNode>(); },
    comb::charP('.'));

// for dividing a char class into ranges
// like [a-cde] -> ([a, c], [d, d], [e, e])
struct ClassItem {
    char start;
    char end;
};

// a single character in character class, either
// - an escaped character, or
// - a character that is not ']'
constexpr auto charClassCharP = comb::alt(
    comb::fmap(&escape_map, comb::right(comb::charP('\\'), comb::any_char)),
    comb::satisfy([](char c) { return c != ']'; }));

// parses a character range
constexpr auto classRangeP = comb::fmap(
    comb::ConstructFromSequence<ClassItem>{},
    comb::separated(charClassCharP, comb::charP('-'), charClassCharP));

// maps a single parsed character to ClassItem
constexpr auto classSingleP =
    comb::fmap([](char c) { return ClassItem{c, c}; }, charClassCharP);

// body of a character class, excluding [ and ]
// character class can be empty
// produces a tuple<bool, vector<ClassItem>>
// -> (<is inverted>, <class contents>)
constexpr auto charClassBodyP = comb::seq(
    comb::optional_or(comb::map_to_value(comb::charP('^'), true), false),
    comb::zero_or_more(comb::alt(classRangeP, classSingleP)));

// parsed a '[' <charClassBody> ']'
// and produces a CharClassNode with inverted and in_class filled
constexpr auto charClassP = comb::fmap(
    [](auto&& tup) -> ASTNodePtr {
        bool inverted = std::get<0>(tup);
        std::vector<ClassItem> items = std::move(std::get<1>(tup));

        std::unique_ptr<CharClassNode> class_node =
            std::make_unique<CharClassNode>(inverted);
        for (auto const& item : items) {
            std::fill(class_node->in_class.begin() + item.start,
                      class_node->in_class.begin() + item.end + 1, true);
        }
        return class_node;
    },
    comb::enclosed(comb::charP('['), charClassBodyP, comb::charP(']')));

// the parsing library does not support states as of now
// without that, I cannot keep track of group IDs like in a normal parser

// A solution is obviously, to create a global variable, and clear it at the end
// of the function. This is arguably the better and cleaner way. All parsers can
// then be made static too, and only have to be initialized once in the whole
// program.

// I have taken the worse path here, just because I wanted to do it without
// global variables. I declare a group ID variable inside the function and then
// capture it in a lambda. The drawback of this is that since the storage
// location of the group_id variable is not fixed, the lambda capture expires
// after the function ends. If the parser that captures it is made static, then
// if parse_regex is called a second time, it will contain a dangling reference
// to the previous `group ID` variable, which is now gone.

// so basically, all these parsers are constructed every time this function is
// called. I will hopefully add a way to handle contexts in the parsing library
// and all this would not be needed.
auto parse_regex(std::string_view pattern) -> ASTNodePtr {
    int group_id = 1;
    // pre-declare the regex parser, since it is recursive
    comb::RecursiveP<ASTNodePtr> regexP;

    static constexpr auto isCapturingP =
        comb::optional_or(comb::map_to_value(comb::stringP("?:"), false), true);

    // parses the "(" and "?:", and returns a group ID if capturing
    auto groupStartP = comb::fmap(
        [&group_id](bool capturing) { return capturing ? group_id++ : -1; },
        comb::right(comb::charP('('), isCapturingP));

    // parses a whole group
    auto groupP = comb::fmap(
        [](std::tuple<int, ASTNodePtr>&& seq) -> ASTNodePtr {
            int group_id = std::get<0>(seq);
            ASTNodePtr regex = std::get<1>(std::move(seq));

            return std::make_unique<GroupNode>(std::move(regex), group_id);
        },
        comb::left(comb::seq(groupStartP, regexP.ref()), comb::charP(')')));

    // a term of a "concatenation", the building block of a regex
    auto termP = comb::alt(wildcardP, literalP, shortCharClassP, charClassP,
                           groupP, anchorP);

    // parses a term and wraps it in a `RepNode` if it is followed by a
    // quantifier
    auto quantifiedTermP = comb::fmap(
        [](auto&& seq) -> ASTNodePtr {
            auto&& [node, quantifier] = std::move(seq);
            if (quantifier)
                return std::make_unique<RepNode>(std::move(node), *quantifier);
            return std::move(node);
        },
        comb::seq(termP,
                  comb::optional_or(
                      comb::fmap([](auto qt) { return std::make_optional(qt); },
                                 quantifierP),
                      std::optional<QuantifierType>(std::nullopt))));

    // concat is a series of qunaitified terms
    // can be empty (for example, patterns like "", "abc||def")
    auto concatP = comb::fmap(
        [](auto&& terms) -> ASTNodePtr {
            return std::make_unique<ConcatNode>(std::move(terms));
        },
        comb::zero_or_more(quantifiedTermP));

    // chains alternation nodes separated by '|'
    auto alternationP = comb::chain(
        concatP, comb::map_to_value(
                     comb::charP('|'),
                     [](ASTNodePtr&& left, ASTNodePtr&& right) -> ASTNodePtr {
                         return std::make_unique<AlternationNode>(
                             std::move(left), std::move(right));
                     }));

    // a regex is a chain of alternations
    regexP.set(alternationP);

    // wrap the regex in a group with ID 0 and enfore the end of string to
    // disallow stray characters
    auto final_parser = comb::fmap(
        [](ASTNodePtr&& regex) -> ASTNodePtr {
            return std::make_unique<GroupNode>(std::move(regex), 0);
        },
        comb::left(regexP, comb::eof));

    auto result = final_parser.parse(pattern);
    if (!result) {
        // that's as good of an error message as I can get from this parsing
        // library as of yet.
        throw std::runtime_error("Regex parsing failed: invalid syntax");
    }

    return std::move(result->first);
}

}  // namespace cppre::detail
