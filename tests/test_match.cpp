#include <cppre/Regex.h>
#include <gtest/gtest.h>

#include <string>
#include <string_view>
using namespace std::string_literals;

#define test_match(pat, s, ret)                               \
    {                                                         \
        std::vector<std::string_view> matches ret;            \
        SCOPED_TRACE("Matching '"s + pat + "' against " + s); \
        cppre::Regex re(pat);                                 \
        auto om = re.match(s);                                \
        ASSERT_TRUE((bool)om);                                \
        auto const& m = *om;                                  \
        ASSERT_EQ(s, m.str());                                \
        for (size_t i = 0; i < m.submatches().size(); ++i) {  \
            ASSERT_EQ(matches[i], m.submatches()[i].str());   \
        }                                                     \
    }

#define test_no_match(pat, str)                                 \
    {                                                           \
        SCOPED_TRACE("Matching '"s + pat + "' against " + str); \
        cppre::Regex re(pat);                                   \
        auto om = re.match(str);                                \
        ASSERT_FALSE((bool)om);                                 \
    }

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
