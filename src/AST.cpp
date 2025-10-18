//
// Created by akul on 8/9/25.
//
#include <cppre/AST.h>
#include <cppre/Color.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>

namespace cppre::detail {
using namespace std::string_literals;

WildcardNode::WildcardNode() : AST(ASTNodeType::Wildcard) {}
auto WildcardNode::print_node(int) const -> std::string {
    return CYAN BOLD "WILDCARD" RESET;
}

RepNode::RepNode(ASTNodePtr&& node, const QuantifierType repType)
    : AST(ASTNodeType::Repetition), node(std::move(node)), repType(repType) {}
auto RepNode::print_node(const int indent_level) const -> std::string {
    std::stringstream ss;

    const char rep = repType == QuantifierType::Plus       ? '+'
                     : repType == QuantifierType::Optional ? '?'
                                                           : '*';
    ss << CYAN BOLD "Repetition" RESET << "(" << YELLOW "'" << rep << "'" RESET
       << ", ";
    ss << node->print_node(indent_level + 1) << ")";
    return ss.str();
}

LiteralNode::LiteralNode(std::string str)
    : AST(ASTNodeType::Literal), value(std::move(str)) {}

auto LiteralNode::print_node(int) const -> std::string {
    std::stringstream ss;

    ss << CYAN BOLD "Literal" RESET << "(<" << YELLOW << value.length()
       << RESET ">, " << MAGENTA BOLD "'" << std::quoted(value) << "'" RESET
       << ")";
    return ss.str();
}

ConcatNode::ConcatNode(std::vector<ASTNodePtr>&& children)
    : AST(ASTNodeType::Concat), children(std::move(children)) {}

auto ConcatNode::print_node(const int indent_level) const -> std::string {
    std::stringstream ss;
    ss << CYAN BOLD "Concat" RESET << "([\n";
    for (const auto& child : children) {
        ss << std::string((indent_level + 1) * 4, ' ')
           << child->print_node(indent_level + 1) << ",\n";
    }
    ss << std::string(indent_level * 4, ' ') << "])";
    return ss.str();
}

AlternationNode::AlternationNode(ASTNodePtr&& left, ASTNodePtr&& right)
    : AST(ASTNodeType::Alternation),
      left(std::move(left)),
      right(std::move(right)) {}

auto AlternationNode::print_node(const int indent_level) const -> std::string {
    std::stringstream ss;
    ss << CYAN BOLD "Alternation" RESET << "(\n";
    ss << std::string((indent_level + 1) * 4, ' ')
       << left->print_node(indent_level + 1) << "\n"
       << std::string((indent_level + 1) * 4, ' ') << "|\n";
    ss << std::string((indent_level + 1) * 4, ' ')
       << right->print_node(indent_level + 1) << "\n";
    ss << std::string(indent_level * 4, ' ') << "])";
    return ss.str();
}

GroupNode::GroupNode(ASTNodePtr&& node, int group_id)
    : AST(ASTNodeType::Group), node(std::move(node)), group_id(group_id) {}

auto GroupNode::print_node(const int indent_level) const -> std::string {
    std::stringstream ss;
    ss << CYAN BOLD "Group" RESET << "(" << YELLOW "<" GREEN;
    if (group_id == -1)
        ss << '?';
    else
        ss << group_id;
    ss << RESET YELLOW ">" RESET ", " << node->print_node(indent_level + 1)
       << ")";
    return ss.str();
}

}  // namespace cppre::detail
