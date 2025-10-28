#ifndef REGEX_H
#define REGEX_H

#include <cppre/AST.h>
#include <cppre/VM.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>
namespace cppre {

class Match {
    std::string_view m_view;
    bool m_matched;
    size_t m_begin;
    size_t m_end;
    std::vector<Match> m_submatches;

    static auto from_saved_array(detail::Thread::SavedArray const& saved,
                                 std::string const& str) -> Match;

    friend class Regex;

   public:
    Match(std::string_view full_str, size_t begin, size_t end);

    Match();

    auto matched() const -> bool;
    auto get_begin() const -> size_t;
    auto get_end() const -> size_t;
    auto str() const -> std::string_view const&;
    auto submatches() const -> std::vector<Match> const&;
};

class Regex {
    std::string m_pattern;
    detail::ASTNodePtr m_ast;
    mutable detail::VM m_vm;

   public:
    Regex(std::string const& pat);

    auto search(std::string const& str) const -> std::optional<Match>;
    auto match(std::string const& str) const -> std::optional<Match>;
    auto search_bool(std::string const& str) const -> bool;
    auto match_bool(std::string const& str) const -> bool;

    auto print_ast() const -> std::string;
    auto print_code() const -> void;
};

}  // namespace cppre
#endif