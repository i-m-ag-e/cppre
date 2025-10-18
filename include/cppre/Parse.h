//
// Created by akul on 8/9/25.
//

#ifndef PARSE_H
#define PARSE_H

#include <cppre/AST.h>

#include <memory>
#include <string_view>

namespace cppre {
namespace detail {
std::unique_ptr<AST> parse_regex(std::string_view pattern);
}
}  // namespace cppre

#endif  // PARSE_H
