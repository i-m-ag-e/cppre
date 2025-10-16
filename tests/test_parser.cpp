#include <cppre/AST.h>
#include <cppre/Parse.h>
#include <gtest/gtest.h>

#include <cctype>
#include <sstream>
#include <string>

namespace {
using namespace cppre::detail;

auto dump_string(std::string const& str) -> std::string {
    std::stringstream ss;
    ss << '\'';
    for (char c : str) {
        if (isspace(c) || c == '\'' || c == '\\')
            ss << '\\' << c;
        else
            ss << c;
    }
    ss << '\'';
    return ss.str();
}

auto dump_test_ast(ASTNodePtr const& node) -> std::string;

auto dump_test_ast(LiteralNode const& node) -> std::string {
    return "Literal(" + dump_string(node.value) + ")";
}

auto dump_test_ast(WildcardNode const&) -> std::string {
    return "Wildcard()";
}

auto dump_test_ast(ConcatNode const& node) -> std::string {
    std::string s;
    for (auto const& n : node.children) {
        s += dump_test_ast(n);
    }
    return s;
}

auto dump_test_ast(AlternationNode const& node) -> std::string {
    return "(" + dump_test_ast(node.left) + "|" + dump_test_ast(node.right) +
           ")";
}

auto dump_test_ast(RepNode const& node) -> std::string {
    std::string s = "Rep(";
    s += node.repType == QuantifierType::Star   ? '*'
         : node.repType == QuantifierType::Plus ? '+'
                                                : '?';
    s += ", " + dump_test_ast(node.node) + ")";
    return s;
}
auto dump_test_ast(GroupNode const& node) -> std::string {
    return "((" + dump_test_ast(node.node) + "))";
}

auto dump_test_ast(ASTNodePtr const& node) -> std::string {
    switch (node->type) {
        case cppre::detail::ASTNodeType::Alternation:
            return dump_test_ast(static_cast<AlternationNode const&>(*node));
        case cppre::detail::ASTNodeType::Literal:
            return dump_test_ast(static_cast<LiteralNode const&>(*node));
        case cppre::detail::ASTNodeType::Wildcard:
            return dump_test_ast(static_cast<WildcardNode const&>(*node));
        case cppre::detail::ASTNodeType::Concat:
            return dump_test_ast(static_cast<ConcatNode const&>(*node));
        case cppre::detail::ASTNodeType::Repetition:
            return dump_test_ast(static_cast<RepNode const&>(*node));
        case cppre::detail::ASTNodeType::Group:
            return dump_test_ast(static_cast<GroupNode const&>(*node));
    }
}
}  // namespace

TEST(ParserTest, TestLiteral) {
    ASSERT_EQ(dump_test_ast(parse_regex("a")), "Literal('a')");
    ASSERT_EQ(dump_test_ast(parse_regex("abcd")), "Literal('abcd')");
    ASSERT_EQ(dump_test_ast(parse_regex("abcd+")),
              "Literal('abc')Rep(+, Literal('d'))");
}
