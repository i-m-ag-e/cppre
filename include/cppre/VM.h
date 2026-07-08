//
// Created by akul on 9/9/25.
//

#ifndef VM_H
#define VM_H
#include <cppre/AST.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace cppre::detail {
enum class InstructionType : uint16_t {
    String,
    Split,
    Save,
    Any,
    Anchor,
    Jump,
    CharClass,
    ShortCharClass,
    InvertedCharClass,
    Match,
};

using Bytecode = std::vector<uint16_t>;

struct Thread {
    using SavedArray = std::vector<int>;
    using SharedSavedArray = std::shared_ptr<SavedArray>;

    size_t pc;
    size_t sp;
    SharedSavedArray saved;

    explicit Thread(size_t ngroups, size_t pc, size_t sp);
    explicit Thread(size_t pc, size_t sp, SharedSavedArray&& old_saved);
    explicit Thread(size_t pc, size_t sp, SharedSavedArray const& old_saved);
    explicit Thread(size_t pc, SharedSavedArray&& old_saved);
    explicit Thread(size_t pc, SharedSavedArray const& old_saved);
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
    auto make_code(CharClassNode const& node) -> void;
    auto make_code(ShortCharClass const& node) -> void;
    auto make_code(AnchorNode const& node) -> void;

    auto add_thread(ThreadList& list, Thread&& new_thread,
                    std::string_view const& str) -> void;
    auto run_thread(ThreadList& list, Thread const& thread,
                    std::string_view const& str) -> bool;

    auto print_threadlist(ThreadList const& tlist) const -> void;
    auto print_inst(int i) const -> int;
    auto print_bytecode() const -> void;

    std::vector<bool> visited;

   public:
    size_t ngroups = 0;
    Bytecode bytecode;
    std::vector<std::string_view> strings;
    std::vector<std::vector<bool>> char_classes;

    VM(ASTNodePtr const&);
    auto run_vm(std::string_view const& str, int pc_offset)
        -> std::optional<Thread::SavedArray>;
    auto print_code(size_t offset = 0) const -> void;
};
}  // namespace cppre::detail

#endif  // VM_H
