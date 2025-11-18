#include <cppre/Regex.h>
#include <gtest/gtest.h>

#include <exception>
#include <string>
#include <string_view>
using namespace std::string_literals;

#define test_match(pat, s, ret)                                             \
    do {                                                                    \
        try {                                                               \
            std::vector<std::string_view> matches ret;                      \
            SCOPED_TRACE("Matching '"s + pat + "' against " + s);           \
            cppre::Regex re(pat);                                           \
            auto om = re.match(s);                                          \
            EXPECT_TRUE((bool)om);                                          \
            auto const& m = *om;                                            \
            EXPECT_EQ(s, m.str());                                          \
            for (size_t i = 0; i < m.submatches().size(); ++i) {            \
                EXPECT_EQ(matches[i], m.submatches()[i].str())              \
                    << "Failed while matching submatch i: " << i;           \
            }                                                               \
        } catch (std::exception const& e) {                                 \
            std::cerr << "Exception while testing against pattern: " << pat \
                      << ", what(): " << e.what() << std::endl;             \
        }                                                                   \
    } while (0)

#define test_no_match(pat, str)                                             \
    do {                                                                    \
        try {                                                               \
            SCOPED_TRACE("Matching '"s + pat + "' against " + str);         \
            cppre::Regex re(pat);                                           \
            auto om = re.match(str);                                        \
            ASSERT_FALSE((bool)om);                                         \
        } catch (std::exception const& e) {                                 \
            std::cerr << "Exception while testing against pattern: " << pat \
                      << ", what(): " << e.what() << std::endl;             \
        }                                                                   \
    } while (0)

TEST(IntegrationTestMatch, TestLiterals) {
    test_match("a", "a", {});
    test_match("abcd", "abcd", {});
}

TEST(IntegrationTestMatch, TestWildcard) {
    test_match(".", "a", {});
    test_match(".", "$", {});
    test_match("ab.cd", "ab@cd", {});
    test_match("ab.cd", "ab_cd", {});
}

TEST(IntegrationTestMatch, TestAlternation) {
    test_match("a|b", "a", {});
    test_match("a|b", "b", {});
    test_no_match("a|b", "c");

    test_match("cat|dog", "cat", {});
    test_match("cat|dog", "dog", {});
    test_no_match("cat|dog", "bird");
    test_no_match("cat|dog", "catdog");

    test_match("apple|apply", "apple", {});
    test_match("apple|apply", "apply", {});

    test_match("a(b|c)d", "abd", {"b"});
    test_match("a(b|c)d", "acd", {"c"});
    test_no_match("a(b|c)d", "ad");

    test_match("a|", "a", {});
    test_match("a|", "", {});
    test_match("|b", "b", {});
    test_match("|b", "", {});
}

TEST(IntegrationTestMatch, TestRepetition) {
    test_match("a*", "", {});
    test_match("a*", "a", {});
    test_match("a*", "aaaa", {});
    test_no_match("a*", "b");
    test_no_match("a*", "aaab");

    test_match("a+", "a", {});
    test_match("a+", "aaaa", {});
    test_no_match("a+", "");
    test_no_match("a+", "aaab");

    test_match("a?", "", {});
    test_match("a?", "a", {});
    test_no_match("a?", "aa");

    test_match("ab*c", "ac", {});
    test_match("ab*c", "abc", {});
    test_match("ab*c", "abbbbc", {});
    test_no_match("ab*c", "abbd");

    test_match("ab+c", "abc", {});
    test_match("ab+c", "abbbbc", {});
    test_no_match("ab+c", "ac");
}

TEST(IntegrationTestMatch, TestGroups) {
    test_match("(a)", "a", {"a"});
    test_match("a(bc)d", "abcd", {"bc"});
    test_no_match("a(bc)d", "ad");

    test_match("(a)*", "", {""});
    test_match("(a)*", "aaaa", {"a"});

    test_match("(ab)+", "ab", {"ab"});
    test_match("(ab)+", "ababab", {"ab"});
    test_no_match("(ab)+", "aba");

    test_match("a(b|c)?d", "ad", {""});
    test_match("a(b|c)?d", "abd", {"b"});
    test_match("a(b|c)?d", "acd", {"c"});
    test_no_match("a(b|c)?d", "abcd");

    test_match("(a)(b)", "ab", ({"a", "b"}));
    test_match("a(b.)c(d*)e", "ab@cddde", ({"b@", "ddd"}));
    test_match("(a(b)c)", "abc", ({"abc", "b"}));
    test_match("a(b(c)?)d", "abcd", ({"bc", "c"}));
    test_match("a(b(c)?)d", "abd", ({"b", ""}));
}

TEST(IntegrationTestMatch, TestComplexPatterns) {
    test_match("a(b|c)*d", "abccbbcd", {"c"});
    test_no_match("a(b|c)*d", "abda");

    test_match("(a.)+", "a1", {"a1"});
    test_match("(a.)+", "a@a-", {"a-"});
    test_no_match("(a.)+", "a");
    test_no_match("(a.)+", "");

    test_match("a(b?c+)+d", "accd", {"cc"});
    test_match("a(b?c+)+d", "abcccd", {"bccc"});
    test_match("a(b?c+)+d", "acbccd", {"bcc"});
    test_no_match("a(b?c+)+d", "abd");
}

TEST(IntegrationTestMatch, TestCharClass) {
    test_match("[abc]", "a", {"a"});
    test_match("[abc]", "c", {"c"});
    test_no_match("[abc]", "d");
    test_no_match("[abc]", "ab");

    test_match("[a-z]", "f", {"f"});
    test_match("[0-9]+", "12345", {"12345"});
    test_match("[a-zA-Z0-9]+", "Test123", {"Test123"});
    test_no_match("[a-z]", "A");
    test_no_match("[0-9]", "a");

    test_match("[^abc]", "d", {"d"});
    test_match("[^a-z]", "A", {"A"});
    test_match("[^0-9]+", "ABC", {"ABC"});
    test_no_match("[^abc]", "a");

    test_match("[a-c-]", "-", {"-"});
    test_match("[a-c-]", "b", {"b"});
    test_match("[-abc]", "-", {"-"});
    test_match("[abc-]", "-", {"-"});
    test_match("[a^b]", "^", {"^"});

    test_match("[\\^abc]", "^", {"^"});
    test_match("[\\-abc]", "-", {"-"});
    test_match("[a\\-c]", "-", {"-"});

    // --- Corrected Tests for ']' ---
    test_match("[a]]", "a]", {"a]"});
    test_match("[\\]ab]", "]", {"]"});
    test_match("[a\\]c]", "]", {"]"});

    // --- Corrected Failing Cases (based on your feedback) ---
    test_no_match("[]ab]", "]");
    test_no_match("[]ab]", "a");
    test_no_match("[^]ab]", "x");
    test_match("[^]ab]", "xab]", {"xab]"});

    test_match("[a-c]+", "abccba", {"abccba"});
    test_match("[^a-c]+", "xyz", {"xyz"});
    test_match("a[a-z]+d", "axyzd", {"axyzd"});
    test_no_match("a[a-z]+d", "a123d");
}

TEST(IntegrationTestMatch, TestShorthandClasses) {
    test_match("\\d", "1", {});
    test_match("\\d", "9", {});
    test_no_match("\\d", "a");
    test_no_match("\\d", " ");

    test_match("\\D", "a", {});
    test_match("\\D", "@", {});
    test_match("\\D", " ", {});
    test_no_match("\\D", "1");

    test_match("\\w", "a", {});
    test_match("\\w", "Z", {});
    test_match("\\w", "5", {});
    test_match("\\w", "_", {});
    test_no_match("\\w", "!");
    test_no_match("\\w", " ");

    test_match("\\W", "!", {});
    test_match("\\W", " ", {});
    test_no_match("\\W", "a");
    test_no_match("\\W", "0");
    test_no_match("\\W", "_");

    test_match("\\s", " ", {});
    test_match("\\s", "\t", {});
    test_match("\\s", "\n", {});
    test_no_match("\\s", "a");

    test_match("\\S", "a", {});
    test_match("\\S", "!", {});
    test_match("\\S", "0", {});
    test_no_match("\\S", " ");

    test_match("\\d+", "123", {});
    test_match("\\w+", "Abc_123", {});
    test_match("\\s+", " \t\n", {});

    test_match("a\\db", "a1b", {});
    // Groups: {"123", "abc"}
    test_match("(\\d+)-(\\w+)", "123-abc", ({"123", "abc"}));
    test_match("\\w+\\s\\d+", "Hello 123", {});

    test_no_match("\\d+", "12a3");
    test_no_match("\\w+", "abc!def");

    // --- Complex Cases ---
    test_match("\\d+\\.\\d+\\.\\d+\\.\\d+", "192.168.1.1", {});

    // Groups: {"count", "42"}
    test_match("\\s*(\\w+)\\s*=\\s*(\\d+)\\s*", "  count = 42  ",
               ({"count", "42"}));

    test_match("\\w+@\\w+\\.\\w+", "user@example.com", {});

    // Groups: {"height:200;"} (captures the last iteration of the group)
    test_match("(\\w+\\s*:\\s*\\d+;)+", "width:100;height:200;",
               {"height:200;"});
}