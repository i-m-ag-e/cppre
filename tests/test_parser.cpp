#include <cppre/AST.h>
#include <cppre/Parse.h>
#include <gtest/gtest.h>

#include <cctype>
#include <exception>
#include <sstream>
#include <string>

using namespace std::string_literals;

namespace {

struct StartupPrinter {
    StartupPrinter() {
#ifdef CPPRE_USE_PARSER_COMB
        std::cout << "\n=== Running tests with ParserComb parser ===\n";
#else
        std::cout << "\n=== Running tests with Handwritten parser ===\n";
#endif
    }
};

StartupPrinter printer;

}  // namespace

namespace {
using namespace cppre::detail;

auto dump_char(char c, std::string_view escapes = "") -> std::string {
    if (escapes.find(c) != std::string::npos)
        return {'\\', c};
    else
        return {c};
}

auto dump_string(std::string const& str, std::string_view escapes = "")
    -> std::string {
    std::string string_escapes = "''";
    string_escapes += escapes;

    std::stringstream ss;
    ss << '\'';
    for (char c : str) {
        ss << dump_char(c, string_escapes);
    }
    ss << '\'';
    return ss.str();
}

auto dump_test_ast(ASTNodePtr const& node) -> std::string;

auto dump_test_ast(LiteralNode const& node) -> std::string {
    return "Literal(" + dump_string(node.value) + ")";
}

auto dump_test_ast(WildcardNode const&) -> std::string {
    return "<WC>";
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
    if (node.group_id == 0)
        return dump_test_ast(node.node);
    return "([" + std::to_string(node.group_id) + "](" +
           dump_test_ast(node.node) + "))";
}

auto dump_test_ast(CharClassNode const& node) -> std::string {
    static constexpr std::string_view kCharClassEscapes = "-()[]^";

    std::stringstream ss;
    ss << "[";
    if (node.inverted)
        ss << "^";

    std::vector<int> idxs;
    for (int i = 0; i < (int)node.in_class.size(); ++i) {
        if (node.in_class[i]) {
            idxs.push_back(i);
        }
    }

    int range_begin = 0;
    for (int i = 0; i < (int)idxs.size(); ++i) {
        if (i < (int)idxs.size() - 1 && idxs[i + 1] - idxs[i] == 1) {
            range_begin = i;
            while (i < (int)idxs.size() - 1 && idxs[i + 1] - idxs[i] == 1)
                ++i;

            if (i - range_begin > 1) {
                ss << "("
                   << dump_char((char)idxs[range_begin], kCharClassEscapes)
                   << "-" << dump_char((char)idxs[i]) << ")";
            } else {
                ss << "(" << dump_char((char)idxs[i - 1], kCharClassEscapes)
                   << ")" << "(" << dump_char((char)idxs[i], kCharClassEscapes)
                   << ")";
            }
        } else {
            ss << "(" << dump_char((char)idxs[i], kCharClassEscapes) << ")";
        }
    }
    ss << "]";
    return ss.str();
}

auto dump_test_ast(ShortCharClass const& scc) -> std::string {
    char c = static_cast<char>(scc.scc_type);
    return {'\\', scc.inverted ? static_cast<char>(std::toupper(c)) : c};
}

auto dump_test_ast(AnchorNode const& node) -> std::string {
    char c = static_cast<char>(node.anchor_type);
    if (node.anchor_type == AnchorNode::AnchorType::StartOfLine ||
        node.anchor_type == AnchorNode::AnchorType::EndOfLine)
        return "Anchor('" + std::string(1, c) + "')";
    return "Anchor('" + std::string{'\\', c} + "')";
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
        case cppre::detail::ASTNodeType::CharClass:
            return dump_test_ast(static_cast<CharClassNode const&>(*node));
        case cppre::detail::ASTNodeType::ShortCharClass:
            return dump_test_ast(static_cast<ShortCharClass const&>(*node));
        case cppre::detail::ASTNodeType::Anchor:
            return dump_test_ast(static_cast<AnchorNode const&>(*node));
    }
    // unreachable
    return "";
}

#define test_eq(pat, repr)                                     \
    do {                                                       \
        SCOPED_TRACE("Testing pattern: "s + pat);              \
        try {                                                  \
            EXPECT_EQ(dump_test_ast(parse_regex(pat)), repr);  \
        } catch (std::exception const& e) {                    \
            ADD_FAILURE() << "Exception on pattern: " << (pat) \
                          << " -- what(): " << e.what();       \
            throw e;                                           \
        }                                                      \
    } while (0)

}  // namespace

TEST(ParserTest, TestLiteral) {
    test_eq("a", "Literal('a')");
    test_eq("abcd", "Literal('abcd')");
    test_eq("ab\\\\cd", "Literal('ab\\cd')");
    test_eq("abcd\\n", "Literal('abcd\n')");
    test_eq("abcd\\b", "Literal('abcd')Anchor('\\b')");  // \b is special
    test_eq("abcd\\r", "Literal('abcd\r')");
    test_eq("abcd\\t", "Literal('abcd\t')");

    test_eq("abcd+", "Literal('abc')Rep(+, Literal('d'))");
    test_eq("ab\\\\cd*", "Literal('ab\\c')Rep(*, Literal('d'))");
    test_eq("ab\\nc?", "Literal('ab\n')Rep(?, Literal('c'))");

    test_eq("ab\\+cd", "Literal('ab+cd')");
    test_eq("ab\\|", "Literal('ab|')");
    test_eq("ab\\*", "Literal('ab*')");
    test_eq("ab\\n+", "Literal('ab')Rep(+, Literal('\n'))");
    test_eq("ab\\\\+cd", "Literal('ab')Rep(+, Literal('\\'))Literal('cd')");

    test_eq("", "");
    test_eq("abc()", "Literal('abc')([1]())");
    test_eq("abc(xyz)", "Literal('abc')([1](Literal('xyz')))");
    test_eq("abc.", "Literal('abc')<WC>");
    test_eq("abc|", "(Literal('abc')|)");

    test_eq("ab\\?cd", "Literal('ab?cd')");
    test_eq("ab\\.cd", "Literal('ab.cd')");
    test_eq("ab\\(cd", "Literal('ab(cd')");
    test_eq("ab\\)cd", "Literal('ab)cd')");

    test_eq("ab\\+cd", "Literal('ab+cd')");
    test_eq("ab\\|", "Literal('ab|')");
    test_eq("ab\\*", "Literal('ab*')");

    test_eq("ab\\zcd", "Literal('abzcd')");
    test_eq("ab\\_cd", "Literal('ab_cd')");

    test_eq("\\+abc", "Literal('+abc')");
    test_eq("abc\\+", "Literal('abc+')");
    test_eq("\\\\", "Literal('\\')");
    test_eq(".", "<WC>");
    test_eq("\\.", "Literal('.')");
}

TEST(ParserTest, TestGroup) {
    test_eq("(a)", "([1](Literal('a')))");
    test_eq("(abc)", "([1](Literal('abc')))");
    test_eq("x(yz)z", "Literal('x')([1](Literal('yz')))Literal('z')");

    test_eq("((a))", "([1](([2](Literal('a')))))");
    test_eq("(a(b)c)", "([1](Literal('a')([2](Literal('b')))Literal('c')))");
    test_eq("a(b(c)d)e",
            "Literal('a')([1](Literal('b')([2](Literal('c')))Literal('d')))"
            "Literal('e')");

    test_eq("(a)(b)", "([1](Literal('a')))([2](Literal('b')))");
    test_eq("a(b)c(d)e",
            "Literal('a')([1](Literal('b')))Literal('c')([2](Literal('d')))"
            "Literal('e')");

    test_eq("(a)+", "Rep(+, ([1](Literal('a'))))");
    test_eq("(abc)*", "Rep(*, ([1](Literal('abc'))))");
    test_eq("(a)?", "Rep(?, ([1](Literal('a'))))");
    test_eq("a(bc)*d", "Literal('a')Rep(*, ([1](Literal('bc'))))Literal('d')");

    test_eq("(a|b)", "([1]((Literal('a')|Literal('b'))))");
    test_eq("x(a|b)y",
            "Literal('x')([1]((Literal('a')|Literal('b'))))Literal('y')");
    test_eq("(a|b)+", "Rep(+, ([1]((Literal('a')|Literal('b')))))");
    test_eq("(a|(b|c))",
            "([1]((Literal('a')|([2]((Literal('b')|Literal('c'))))))"
            ")");

    test_eq("(a.)", "([1](Literal('a')<WC>))");
    test_eq("(a.*)", "([1](Literal('a')Rep(*, <WC>)))");

    test_eq("()", "([1]())");
    test_eq("a()b", "Literal('a')([1]())Literal('b')");
    test_eq("()*", "Rep(*, ([1]()))");

    test_eq("(a(b|c)*d)+",
            "Rep(+, ([1](Literal('a')Rep(*, "
            "([2]((Literal('b')|Literal('c')))))Literal('d'))))");
    test_eq("(a)|(b)", "(([1](Literal('a')))|([2](Literal('b'))))");
}

TEST(ParserTest, TestAlternation) {
    test_eq("a|b", "(Literal('a')|Literal('b'))");
    test_eq("abc|xyz", "(Literal('abc')|Literal('xyz'))");
    test_eq("a|bc", "(Literal('a')|Literal('bc'))");
    test_eq("a(b|c)d",
            "Literal('a')([1]((Literal('b')|Literal('c'))))Literal('d')");
    test_eq("(a|b)*", "Rep(*, ([1]((Literal('a')|Literal('b')))))");
    test_eq("a*|b+", "(Rep(*, Literal('a'))|Rep(+, Literal('b')))");
    test_eq("a|", "(Literal('a')|)");
    test_eq("|b", "(|Literal('b'))");
    test_eq("(a)|(b)", "(([1](Literal('a')))|([2](Literal('b'))))");
}

TEST(ParserTest, RepetitionTests) {
    // --- Basic Repetition on Literals ---
    test_eq("a*", "Rep(*, Literal('a'))");
    test_eq("a+", "Rep(+, Literal('a'))");
    test_eq("a?", "Rep(?, Literal('a'))");

    test_eq("abc*", "Literal('ab')Rep(*, Literal('c'))");
    test_eq("abc+", "Literal('ab')Rep(+, Literal('c'))");
    test_eq("abc?", "Literal('ab')Rep(?, Literal('c'))");

    test_eq("(abc)*", "Rep(*, ([1](Literal('abc'))))");
    test_eq("(a|b)+", "Rep(+, ([1]((Literal('a')|Literal('b')))))");
    test_eq("(a)?b", "Rep(?, ([1](Literal('a'))))Literal('b')");

    test_eq(".*", "Rep(*, <WC>)");
    test_eq(".+", "Rep(+, <WC>)");
    test_eq("a.?", "Literal('a')Rep(?, <WC>)");

    test_eq("\\**", "Rep(*, Literal('*'))");
    test_eq("\\++", "Rep(+, Literal('+'))");
    test_eq("\\??", "Rep(?, Literal('?'))");
}

TEST(ParserTest, CharClassTests) {
    // --- Basic Classes ---
    test_eq("[a]", "[(a)]");
    test_eq("[ab]", "[(a)(b)]");
    test_eq("[abc]", "[(a-c)]");
    test_eq("[ace]", "[(a)(c)(e)]");

    // --- Ranges ---
    test_eq("[a-c]", "[(a-c)]");
    test_eq("[a-b]", "[(a)(b)]");
    test_eq("[0-9]", "[(0-9)]");
    test_eq("[a-zA-Z]", "[(A-Z)(a-z)]");

    // --- Combinations ---
    test_eq("[a-c_]", "[(_)(a-c)]");
    test_eq("[_a-c]", "[(_)(a-c)]");
    test_eq("[a-c0-2]", "[(0-2)(a-c)]");

    // --- Inverted Classes ---
    test_eq("[^a]", "[^(a)]");
    test_eq("[^ab]", "[^(a)(b)]");
    test_eq("[^a-c]", "[^(a-c)]");
    test_eq("[^ace]", "[^(a)(c)(e)]");

    // --- Positional Metacharacters (as literals) ---
    test_eq("[^]", "[^]");
    test_eq("[^\\^]", "[^(\\^)]");
    test_eq("[a^b]", "[(\\^)(a)(b)]");
    test_eq("[-]", "[(\\-)]");
    test_eq("[a-]", "[(\\-)(a)]");
    test_eq("[-a]", "[(\\-)(a)]");
    test_eq("[]]", "[]Literal(']')");
    test_eq("[a]]", "[(a)]Literal(']')");
    // EXPECT_THROW(test_eq("[[]", "[(\\[)]"));

    // --- Positional Metacharacters (inverted) ---
    test_eq("[^-]", "[^(\\-)]");
    test_eq("[^]]", "[^]Literal(']')");
    test_eq("[^[]", "[^(\\[)]");

    // --- Metacharacters that are literals inside [...] ---
    test_eq("[.?*+]", "[(*)(+)(.)(?)]");

    // --- Escaped Metacharacters (dumper escapes these) ---
    test_eq("[()]", "[(\\()(\\))]");

    // --- Escaping (parser-side) ---
    test_eq("[\\^]", "[(\\^)]");
    test_eq("[\\]]", "[(\\])]");
    test_eq("[\\[]", "[(\\[)]");
    test_eq("[\\-]", "[(\\-)]");
    test_eq("[a\\-b]", "[(\\-)(a)(b)]");
    test_eq("[a\\\\b]", "[(\\)(a)(b)]");

    // --- Range Edge Cases (based on dumper logic) ---
    test_eq("[--A]", "[(\\--A)]");
    test_eq("[--]", "[(\\-)]");
    test_eq("[---]", "[(\\-)]");
    test_eq("[a-c-]", "[(\\-)(a-c)]");
}

TEST(ParserTest, ShortCharClassTests) {
    test_eq("\\d", "\\d");
    test_eq("\\D", "\\D");
    test_eq("\\w", "\\w");
    test_eq("\\W", "\\W");
    test_eq("\\s", "\\s");
    test_eq("\\S", "\\S");

    test_eq("a\\db", "Literal('a')\\dLiteral('b')");
    test_eq("a\\Db", "Literal('a')\\DLiteral('b')");
    test_eq("\\w\\s\\d", "\\w\\s\\d");
    test_eq("a\\Wb", "Literal('a')\\WLiteral('b')");

    test_eq("\\d*", "Rep(*, \\d)");
    test_eq("\\w+", "Rep(+, \\w)");
    test_eq("\\s?", "Rep(?, \\s)");
    test_eq("\\D+", "Rep(+, \\D)");
    test_eq("a\\s*b", "Literal('a')Rep(*, \\s)Literal('b')");

    test_eq("\\d|a", "(\\d|Literal('a'))");
    test_eq("a|\\s", "(Literal('a')|\\s)");
    test_eq("\\w|\\D", "(\\w|\\D)");

    test_eq("(\\d)", "([1](\\d))");
    test_eq("(a\\s)+", "Rep(+, ([1](Literal('a')\\s)))");
    test_eq("(\\d|\\w)*", "Rep(*, ([1]((\\d|\\w))))");
}

TEST(ParserTest, AnchorTests) {
    test_eq("^a", "Anchor('^')Literal('a')");
    test_eq("a$", "Literal('a')Anchor('$')");
    test_eq("\\Aa\\Z", "Anchor('\\A')Literal('a')Anchor('\\Z')");
    test_eq("a\\Ab^c$d\\be\\Zf\\Bg",
            "Literal('a')Anchor('\\A')Literal('b')Anchor('^')"
            "Literal('c')Anchor('$')Literal('d')Anchor('\\b')"
            "Literal('e')Anchor('\\Z')Literal('f')Anchor('\\B')Literal('g')");

    // \b inside a character class is \x08, but outside is an anchor
    test_eq("\\bword\\B", "Anchor('\\b')Literal('word')Anchor('\\B')");
    test_eq("[\\b]", "[(\b)]");
    test_eq("\\b[\\b]", "Anchor('\\b')[(\b)]");

    test_eq("\\^a", "Literal('^a')");
    test_eq("a\\$", "Literal('a$')");

    test_eq("\\\\A", "Literal('\\A')");
    test_eq("\\\\Z", "Literal('\\Z')");
    test_eq("\\\\b", "Literal('\\b')");
    test_eq("\\\\B", "Literal('\\B')");

    test_eq("^\\^$", "Anchor('^')Literal('^')Anchor('$')");
    test_eq("\\A\\\\A", "Anchor('\\A')Literal('\\A')");
    test_eq("\\b\\\\b", "Anchor('\\b')Literal('\\b')");
}
