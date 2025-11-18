#include <cppre/Regex.h>
#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <vector>
using namespace std::string_literals;

#define test_search(pat, s, ret)                                \
    {                                                           \
        std::vector<std::string_view> matches ret;              \
        std::string ss(s);                                      \
        SCOPED_TRACE("Searching '"s + pat + "' against " + s);  \
        cppre::Regex re(pat);                                   \
        auto om = re.search(ss);                                \
        EXPECT_TRUE((bool)om);                                  \
        auto const& m = *om;                                    \
        EXPECT_EQ(matches.size(), m.submatches().size() + 1);   \
        print_match(m);                                         \
        EXPECT_EQ(matches[0], m.str());                         \
        for (size_t i = 1; i < m.submatches().size(); ++i) {    \
            EXPECT_EQ(matches[i], m.submatches()[i - 1].str()); \
        }                                                       \
    }

#define test_no_search(pat, str)                                 \
    {                                                            \
        SCOPED_TRACE("Searching '"s + pat + "' against " + str); \
        cppre::Regex re(pat);                                    \
        auto om = re.search(str);                                \
        EXPECT_FALSE((bool)om);                                  \
    }

auto print_match(cppre::Match const& match) -> void {
    if (match.submatches().size() > 0) {
        std::cout << "Match { \n"
                  << "    begin: " << match.get_begin() << ",\n"
                  << "    end:   " << match.get_end() << ",\n"
                  << "    match: '" << match.str() << "',\n"
                  << "    submatches: (" << match.submatches().size()
                  << ") [\n";
        for (size_t i = 0; i < match.submatches().size(); ++i) {
            std::cout << "        ";
            print_match(match.submatches()[i]);
            std::cout << ",\n";
        }
        std::cout << "    ]\n}";
    } else {
        std::cout << "Match< '" << match.str() << "'; (" << match.get_begin()
                  << ", " << match.get_end() << ")>";
    }
}

TEST(IntegrationSearchTest, TestLiterals) {
    test_search("a", "b_a_c", {"a"});
    test_search("bc", "abcd", {"bc"});
    test_search("ab", "abcd", {"ab"});
    test_no_search("x", "abc");
    test_no_search("abc", "ab");
}

TEST(IntegrationSearchTest, TestWildcard) {
    test_search(".", "abc", {"a"});
    test_search("b.d", "ab_de", {"b_d"});
    test_search("b.d", "prefix_bcd_suffix", {"bcd"});
    test_no_search("a.c", "ab_c");
}

TEST(IntegrationSearchTest, TestAlternation) {
    test_search("cat|dog", "I have a dog.", {"dog"});
    test_search("cat|dog", "This is a catastrophe.", {"cat"});
    test_no_search("cat|dog", "My pet is a bird.");

    test_search("a(b|c)d", "prefix_acd_suffix", ({"acd", "c"}));
    test_search("a(b|c)d", "prefix_abd_suffix", ({"abd", "b"}));
    test_no_search("a(b|c)d", "prefix_ad_suffix");
}

TEST(IntegrationSearchTest, TestRepetition) {
    test_search("a*", "bbb", {""});
    test_search("a*", "", {""});

    test_search("a+", "bbb_aaa_ccc", {"aaa"});
    test_no_search("a+", "bbb_ccc");

    test_search("ab*c", "zzzaczzz", {"ac"});
    test_search("ab*c", "zzzabcdzzz", {"abc"});
    test_search("ab+c", "zzzabcdzzz", {"abc"});
    test_no_search("ab+c", "zzzaczzz");
}

TEST(IntegrationSearchTest, TestGroups) {
    test_search("(a)", "b_a_c", ({"a", "a"}));
    test_search("a(bc)d", "xyz_abcd_xyz", ({"abcd", "bc"}));
    test_no_search("a(bc)d", "xyz_ad_xyz");

    test_search("(a)*", "bbbaaaaccc", ({"", ""}));
    test_search("(ab)+", "prefix_ababab_suffix", ({"ababab", "ab"}));

    test_search("a(b|c)?d", "xyz_ad_123", ({"ad", ""}));
    test_search("a(b|c)?d", "xyz_abd_123", ({"abd", "b"}));
    test_search("a(b|c)?d", "xyz_acd_123", ({"acd", "c"}));

    test_search("(a)(b)", "prefix_ab_suffix", ({"ab", "a", "b"}));
    test_search("a(b.)c(d*)e", "prefix_ab@cddde_suffix",
                ({"ab@cddde", "b@", "ddd"}));
    test_search("(a(b)c)", "prefix_abc_suffix", ({"abc", "abc", "b"}));
    test_search("a(b(c)?)d", "prefix_abcd_suffix", ({"abcd", "bc", "c"}));
    test_search("a(b(c)?)d", "prefix_abd_suffix", ({"abd", "b", ""}));
}

TEST(IntegrationSearchTest, TestComplexPatterns) {
    test_search("a(b|c)*d", "xyz_abccbbcd_xyz", ({"abccbbcd", "c"}));
    test_search("a(b|c)*d", "xyz_ad_xyz", ({"ad", ""}));
    test_search("a(b|c)*d", "xyz_abd_a_xyz", ({"abd", "b"}));

    test_search("(a.)+", "prefix_a1a2_suffix", ({"a1a2", "a2"}));
    test_search("(a.)+", "prefix_a_suffix", ({"a_", "a_"}));
    test_no_search("(a.)+", "prefix_a");

    test_search("a(b?c+)+d", "prefix_acbccd_suffix", ({"acbccd", "bcc"}));
    test_no_search("a(b?c+)+d", "prefix_abd_suffix");

    test_search("(a(b(c)d)e)", "prefix_abcde_suffix",
                ({"abcde", "abcde", "bcd", "c"}));
    test_search("a((b)|(c))*d", "prefix_abccbbcd_suffix",
                ({"abccbbcd", "c", "b", "c"}));
    test_search("(a.)+|(b.)+", "prefix_b1b2_suffix", ({"b1b2", "", "b2"}));
    test_search("(a.)+|(b.)+", "prefix_a1a2_suffix", ({"a1a2", "a2", ""}));
    test_search("a((b?c+)+)d", "prefix_acbccd_suffix",
                ({"acbccd", "cbcc", "bcc"}));
}

TEST(IntegrationSearchTest, TestCharClass) {
    test_search("[abc]", "z_c_z", {"c"});
    test_no_search("[abc]", "xyz");

    test_search("[a-z]", "123_f_789", {"f"});
    test_search("[0-9]+", "abc123def", {"123"});
    test_search("[a-zA-Z0-9]+", "!!Test123!!", {"Test123"});
    test_no_search("[a-z]", "123_A_456");

    test_search("[^abc]", "aaabd", {"d"});
    test_search("[^a-z]", "abcA", {"A"});
    test_search("[^0-9]+", "123ABC456", {"ABC"});
    test_no_search("[^abc]", "abc");

    test_search("[a-c-]", "xyz-", {"-"});
    test_search("[a-c-]", "xyzb", {"b"});
    test_search("[-abc]", "xyz-", {"-"});
    test_search("[abc-]", "xyz-", {"-"});
    test_search("[a^b]", "z^z", {"^"});

    test_search("[\\^abc]", "z^z", {"^"});
    test_search("[\\-abc]", "z-z", {"-"});

    // --- Corrected Tests for ']' ---
    test_search("[\\]ab]", "z]z", {"]"});
    test_search("[a]]", "xxxa]xxx", {"a]"});

    // --- Corrected Failing Cases (based on your feedback) ---
    test_no_search("[]ab]", "z]z");
    test_no_search("[]ab]", "zaz");
    test_no_search("[^]ab]", "xyz");
    test_search("[^]ab]", "xyz_aab]_xyz", {"aab]"});

    test_search("[a-c]+", "xyzabccbaxyz", {"abccba"});
    test_search("[^a-c]+", "abcxyzabc", {"xyz"});
    test_search("a[a-z]+d", "123axyzd456", {"axyzd"});
    test_no_search("a[a-z]+d", "123a123d456");
}

TEST(IntegrationSearchTest, TestShorthandClasses) {
    test_search("\\d", "abc1def", {"1"});
    test_no_search("\\d", "abcdef");

    test_search("\\D", "123a456", {"a"});
    test_no_search("\\D", "123456");

    test_search("\\w", "!@#a$%", {"a"});
    test_search("\\w", "!@#_$%", {"_"});
    test_no_search("\\w", "!@#$%^&*()");

    test_search("\\W", "abc!def", {"!"});
    test_search("\\W", "abc def", {" "});
    test_no_search("\\W", "abcdef123_");

    test_search("\\s", "abc def", {" "});
    test_search("\\s", "abc\tdef", {"\t"});
    test_no_search("\\s", "abcdef");

    test_search("\\S", "   a   ", {"a"});
    test_no_search("\\S", "   \t\n   ");

    test_search("\\d+", "abc123def", {"123"});
    test_search("\\w+", "!!!Hello_World!!!", {"Hello_World"});
    test_search("a\\s+b", "test a   b test", {"a   b"});

    test_search("(\\d+)-(\\w+)", "id: 42-test_case",
                ({"42-test_case", "42", "test_case"}));
    test_search("\\w+\\d\\d", "User99", {"User99"});

    // --- Complex Cases ---
    test_search("Price:\\s*\\$\\d+", "Total Price: $500 inclusive",
                {"Price: $500"});

    test_search("key\\s*=\\s*'(\\w+)'", "config { key = 'secret_value' }",
                ({"key = 'secret_value'", "secret_value"}));

    test_search("(\\d\\d\\d\\d)-(\\d\\d)-(\\d\\d)",
                "Log entry: 2025-10-27 error...",
                ({"2025-10-27", "2025", "10", "27"}));

    test_search("call\\s+(\\w+)\\((\\d+)\\)", "Please call process(123) now",
                ({"call process(123)", "process", "123"}));
}
