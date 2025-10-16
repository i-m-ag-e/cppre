//
// Created by akul on 8/9/25.
//

#include <cppre/AST.h>
#include <cppre/Parse.h>
#include <cppre/VM.h>

#include <iostream>

int main(int argc, char* argv[]) {
    using namespace cppre::detail;

    const std::string pattern = argv[1];
    auto ast = parse_regex(pattern);
    std::cout << ast->print_node(0) << std::endl;
    VM vm(ast);
    vm.print_code();

    const std::string str = argv[2];
    std::cout << (vm.run_vm(str) ? "true" : "false") << std::endl;
}
