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

enum class QuantifierType : char { Star = '*', Plus = '+', Optional = '?' };

enum class ASTNodeType {
    Wildcard,
    Repetition,
    Literal,
    Concat,
    Alternation,
    Group,
    CharClass,
    ShortCharClass,
    Anchor
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

struct CharClassNode final : public AST {
    static constexpr size_t kCharDomainLimit = 255;

    std::vector<bool> in_class;
    bool inverted;

    explicit CharClassNode(bool inverted);
    auto print_node(int indent_level) const -> std::string override;
    ~CharClassNode() override = default;
};

struct ShortCharClass final : public AST {
    enum class SCCType : char { Digit = 'd', Space = 's', Word = 'w' };

    SCCType scc_type;
    bool inverted;

    explicit ShortCharClass(char c);
    auto print_node(int indent_level) const -> std::string override;
    ~ShortCharClass() override = default;
};

struct AnchorNode final : public AST {
    enum class AnchorType : char {
        StartOfString = 'A',
        StartOfLine = '^',
        EndOfString = 'Z',
        EndOfLine = '$',
        WordBoundary = 'b',
        NotWordBoundary = 'B'
    };

    AnchorType anchor_type;

    explicit AnchorNode(AnchorType anchor_type);
    auto print_node(int indent_level) const -> std::string override;
    ~AnchorNode() override = default;
};

}  // namespace detail
}  // namespace cppre

#endif  // AST_H
