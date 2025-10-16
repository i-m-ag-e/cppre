//
// Created by akul on 9/9/25.
//

#ifndef VM_H
#define VM_H
#include <cppre/AST.h>

#include <cstdint>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace cppre::detail {
enum class InstructionType : uint16_t { String, Split, Save, Any, Jump, Match };
auto inst2string(InstructionType type) -> std::string_view;

using Bytecode = std::vector<uint16_t>;

struct Thread {
    size_t pc;
    size_t sp;
    std::vector<uint16_t> saved;

    explicit Thread(size_t ngroups) : pc(0), sp(0), saved(ngroups, 0) {}
    explicit Thread(size_t pc, size_t sp,
                    std::vector<uint16_t> const& old_saved)
        : pc(pc), sp(sp), saved(old_saved) {}
    explicit Thread(size_t pc, std::vector<uint16_t> const& old_saved)
        : pc(pc), saved(old_saved) {}
};
using ThreadList = std::vector<Thread>;

struct VM {
   private:
    auto from_ast(ASTNodePtr const& ast) -> void;
    auto make_code(LiteralNode const& node) -> void;
    auto make_code(WildcardNode const& node) -> void;
    auto make_code(RepNode const& node) -> void;
    auto make_code(ConcatNode const& node) -> void;
    auto make_code(AlternationNode const& node) -> void;
    auto make_code(GroupNode const& node) -> void;

    auto add_thread(ThreadList& list, Thread&& new_thread) -> void;
    auto run_thread(ThreadList& list, Thread const& thread,
                    std::string_view const& str) -> bool;

    auto print_bytecode() const -> void;

    std::unordered_set<size_t> marked_insts;

   public:
    size_t ngroups = 0, proglen = 0;
    Bytecode bytecode;
    std::vector<std::string_view> strings;

    VM(ASTNodePtr const&);
    auto run_vm(std::string_view const& str) -> bool;
    auto print_code() const -> void;
};
}  // namespace cppre::detail

#endif  // VM_H
