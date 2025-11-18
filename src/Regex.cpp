#include <cppre/Parse.h>
#include <cppre/Regex.h>
#include <cppre/VM.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cppre/AST.h"

namespace cppre {
using namespace detail;

Match::Match(std::string_view full_str, size_t begin, size_t end)
    : m_string(full_str.begin() + begin, full_str.begin() + end),
      m_matched(true),
      m_begin(begin),
      m_end(end) {}

Match::Match() : m_matched(false) {}

auto Match::matched() const -> bool {
    return m_matched;
}

auto Match::get_begin() const -> size_t {
    return m_begin;
}

auto Match::get_end() const -> size_t {
    return m_end;
}

auto Match::str() const -> std::string const& {
    return m_string;
}

auto Match::submatches() const -> std::vector<Match> const& {
    return m_submatches;
}

auto Match::from_saved_array(detail::Thread::SavedArray const& saved,
                             std::string const& str) -> Match {
    auto root_match = Match(str, saved[0], saved[1]);

    for (size_t i = 2; i < saved.size(); i += 2) {
        if (saved[i] == -1)
            root_match.m_submatches.push_back(Match());
        else
            root_match.m_submatches.push_back(
                Match(str, saved[i], saved[i + 1]));
    }
    return root_match;
}

Regex::Regex(std::string const& pat)
    : m_pattern(pat), m_ast(parse_regex(pat)), m_vm(m_ast) {}

namespace {
constexpr size_t kMatchPCOffset = 6;
constexpr size_t kSearchPCOffset = 0;
}  // namespace

auto Regex::search_bool(std::string const& str) const -> bool {
    m_vm.print_code(kSearchPCOffset);
    return (bool)m_vm.run_vm(str, kSearchPCOffset);
}

auto Regex::search(std::string const& str) const -> std::optional<Match> {
    // m_vm.print_code(kSearchPCOffset);

    auto saved = m_vm.run_vm(str, kSearchPCOffset);
    if (saved)
        return std::make_optional(Match::from_saved_array(*saved, str));
    return {};
}

auto Regex::match_bool(std::string const& str) const -> bool {
    // m_vm.print_code();

    auto saved = m_vm.run_vm(str, kMatchPCOffset);
    return saved && (*saved)[1] == (int)str.length();
}

auto Regex::match(std::string const& str) const -> std::optional<Match> {
    // m_vm.print_code();

    auto saved = m_vm.run_vm(str, kMatchPCOffset);
    if (saved && (*saved)[1] == (int)str.length())
        return std::make_optional(Match::from_saved_array(*saved, str));
    return {};
}

auto Regex::print_ast() const -> std::string {
    return m_ast->print_node(0);
}

auto Regex::print_code() const -> void {
    m_vm.print_code();
}

}  // namespace cppre