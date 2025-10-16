//
// Created by akul on 8/9/25.
//

#ifndef PARSE_H
#define PARSE_H

#include <cppre/AST.h>

#include <memory>

namespace cppre {
namespace detail {
std::unique_ptr<AST> parse_regex(const std::string& pattern);
}
}  // namespace cppre

#endif  // PARSE_H
