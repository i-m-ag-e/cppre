#include <cppre/Regex.h>
#include <gtest/gtest.h>

#define test_search(pat, str) \
    { ASSERT_EQ(cppre::Regex(pat).search_bool(str), true); }
#define test_no_search(pat, str) \
    { ASSERT_EQ(cppre::Regex(pat).search_bool(str), false); }

TEST(IntegrationTestSearch, TestLiterals) {
    test_search("a", "b_a_c");
    test_search("bc", "abcd");
    test_search("ab", "abcd");
    test_no_search("x", "abc");
    test_no_search("abc", "ab");
}

TEST(IntegrationTestSearch, TestWildcard) {
    test_search(".", "abc");
    test_search("b.d", "ab_de");
    test_search("b.d", "prefix_bcd_suffix");
    test_no_search("a.c", "ab_c");
}

TEST(IntegrationTestSearch, TestAlternation) {
    test_search("cat|dog", "I have a dog.");
    test_search("cat|dog", "This is a catastrophe.");
    test_no_search("cat|dog", "My pet is a bird.");

    test_search("a(b|c)d", "prefix_acd_suffix");
    test_search("a(b|c)d", "prefix_abd_suffix");
    test_no_search("a(b|c)d", "prefix_ad_suffix");
}

TEST(IntegrationTestSearch, TestRepetition) {
    test_search("a*", "bbb");
    test_search("a*", "");

    test_search("a+", "bbb_aaa_ccc");
    test_no_search("a+", "bbb_ccc");

    test_search("ab*c", "zzzaczzz");
    test_search("ab*c", "zzzabcdzzz");
    test_search("ab*c", "zzzabcdzzz");
    test_search("ab+c", "zzzabcdzzz");
    test_no_search("ab+c", "zzzaczzz");
}

TEST(IntegrationTestSearch, TestGroups) {
    test_search("(ab)+", "prefix_ababab_suffix");
    test_search("(ab)+", "ab");
    test_no_search("(ab)+", "prefix_a_suffix");

    test_search("a(b|c)?d", "xyz_ad_123");
    test_search("a(b|c)?d", "xyz_abd_123");
    test_search("a(b|c)?d", "xyz_acd_123");
}

TEST(IntegrationTestSearch, TestComplexPatterns) {
    test_search("a(b|c)*d", "xyz_abccbbcd_xyz");
    test_search("a(b|c)*d", "xyz_ad_xyz");
    test_no_search("a(b|c)*d", "ac_bd");

    test_search("(a.)+", "prefix_a1a2_suffix");
    test_search("(a.)+", "prefix_a_suffix");
    test_no_search("(a.)+", "prefix_a");

    test_search("a(b?c+)+d", "prefix_acbccd_suffix");
    test_no_search("a(b?c+)+d", "prefix_abd_suffix");
}