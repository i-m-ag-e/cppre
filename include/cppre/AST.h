//
// Created by akul on 8/9/25.
//

#ifndef AST_H
#define AST_H

#include <memory>
#include <string>
#include <vector>

namespace cppre {
namespace detail {
enum class QuantifierType { Star, Plus, Optional };

enum class ASTNodeType {
    Wildcard,
    Repetition,
    Literal,
    Concat,
    Alternation,
    Group
};

struct AST {
    ASTNodeType type;
    explicit AST(const ASTNodeType type) : type(type) {}
    virtual auto print_node(int indent_level) const -> std::string = 0;
    virtual ~AST() = default;
};
using ASTNodePtr = std::unique_ptr<AST>;

struct WildcardNode final : public AST {
    WildcardNode();

    [[nodiscard]] auto print_node(int indent_level) const
        -> std::string override;
    ~WildcardNode() override = default;
};

struct RepNode final : public AST {
    ASTNodePtr node;
    QuantifierType repType;

    explicit RepNode(ASTNodePtr&& node, QuantifierType repType);
    [[nodiscard]] auto print_node(int indent_level) const
        -> std::string override;
    ~RepNode() override = default;
};

struct LiteralNode final : public AST {
    std::string value;
    explicit LiteralNode(std::string str);
    [[nodiscard]] auto print_node(int indent_level) const
        -> std::string override;
    ~LiteralNode() override = default;
};

struct ConcatNode final : public AST {
    std::vector<ASTNodePtr> children;
    explicit ConcatNode(std::vector<ASTNodePtr>&& children);
    [[nodiscard]] auto print_node(int indent_level) const
        -> std::string override;
    ~ConcatNode() override = default;
};

struct AlternationNode final : public AST {
    ASTNodePtr left;
    ASTNodePtr right;
    explicit AlternationNode(ASTNodePtr&& left, ASTNodePtr&& right);
    [[nodiscard]] auto print_node(int indent_level) const
        -> std::string override;
    ~AlternationNode() override = default;
};

struct GroupNode final : public AST {
    ASTNodePtr node;
    int group_id;

    explicit GroupNode(ASTNodePtr&& node, int group_id);
    auto print_node(int indent_level) const -> std::string override;
    ~GroupNode() override = default;
};

}  // namespace detail
}  // namespace cppre

#endif  // AST_H
