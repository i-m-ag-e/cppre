//
// Created by akul on 8/9/25.
//

#include <cppre/Parse.h>
#include <cppre/Regex.h>
#include <cppre/VM.h>

#include <iostream>

namespace {
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
}  // namespace

int main(int, char* argv[]) {
    using namespace cppre::detail;

    const std::string pattern = argv[1];
    const std::string str = argv[2];
    const cppre::Regex re(pattern);
    std::cout << "Testing \"" << str << "\" against \"" << pattern << "\"\n";
    std::cout << re.print_ast() << std::endl;
    re.print_code();

    auto match_res = re.match(str);
    if (match_res) {
        print_match(*match_res);
    } else {
        std::cout << "No MATCH\n";
    }
    std::cout << '\n';

    auto search_res = re.search(str);
    if (search_res) {
        print_match(*search_res);
    } else {
        std::cout << "No SEARCH\n";
    }
    std::cout << '\n';

    const cppre::Regex re_new("^hello$");
    const std::string test = "hello";
    std::cout << re_new.print_ast() << std::endl;
    auto match1 = re_new.match(test);
    if (match1) {
        print_match(*match1);
    } else {
        std::cout << "No MATCH\n";
    }
    std::cout << '\n';

    auto search1 = re_new.search(test);
    if (search1) {
        print_match(*search1);
    } else {
        std::cout << "No SEARCH\n";
    }

    std::cout << '\n';
}
