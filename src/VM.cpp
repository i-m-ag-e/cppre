//
// Created by akul on 9/9/25.
//

#include <cppre/AST.h>
#include <cppre/Color.h>
#include <cppre/VM.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace cppre {
namespace detail {

auto VM::from_ast(const ASTNodePtr& ast) -> void {
    switch (ast->type) {
        case ASTNodeType::Literal:
            make_code(static_cast<LiteralNode const&>(*ast));
            break;
        case ASTNodeType::Wildcard:
            make_code(static_cast<WildcardNode const&>(*ast));
            break;
        case ASTNodeType::Repetition:
            make_code(static_cast<RepNode const&>(*ast));
            break;
        case ASTNodeType::Concat:
            make_code(static_cast<ConcatNode const&>(*ast));
            break;
        case ASTNodeType::Alternation:
            make_code(static_cast<AlternationNode const&>(*ast));
            break;
        case ASTNodeType::Group:
            make_code(static_cast<GroupNode const&>(*ast));
            break;
        case ASTNodeType::CharClass:
            make_code(static_cast<CharClassNode const&>(*ast));
            break;
    }
}

auto VM::make_code(WildcardNode const&) -> void {
    bytecode.push_back(static_cast<uint16_t>(InstructionType::Any));
}

auto VM::make_code(LiteralNode const& node) -> void {
    bytecode.push_back(static_cast<uint16_t>(InstructionType::String));
    bytecode.push_back(static_cast<uint16_t>(strings.size()));
    strings.push_back(std::string_view(node.value));
}

auto VM::make_code(GroupNode const& node) -> void {
    if (node.group_id != -1) {
        bytecode.push_back(static_cast<uint16_t>(InstructionType::Save));
        bytecode.push_back(2 * node.group_id);
        ngroups = node.group_id + 1;
    }
    from_ast(node.node);
    if (node.group_id != -1) {
        bytecode.push_back(static_cast<uint16_t>(InstructionType::Save));
        bytecode.push_back(2 * node.group_id + 1);
    }
}

auto VM::make_code(AlternationNode const& node) -> void {
    bytecode.push_back(static_cast<uint16_t>(InstructionType::Split));
    const size_t split_pos = bytecode.size() - 1;
    bytecode.push_back(split_pos + 3);
    bytecode.push_back(0);

    from_ast(node.left);
    const size_t jump_pos = bytecode.size();
    bytecode.push_back(static_cast<uint16_t>(InstructionType::Jump));
    bytecode.push_back(0);
    bytecode[split_pos + 2] = static_cast<uint16_t>(bytecode.size());

    from_ast(node.right);
    bytecode[jump_pos + 1] = static_cast<uint16_t>(bytecode.size());
}

auto VM::make_code(ConcatNode const& node) -> void {
    for (const auto& child : node.children) {
        from_ast(child);
    }
}

auto VM::make_code(RepNode const& node) -> void {
    switch (node.repType) {
        case cppre::detail::QuantifierType::Plus: {
            const size_t jump_pos = bytecode.size();
            from_ast(node.node);
            bytecode.push_back(static_cast<uint16_t>(InstructionType::Split));
            bytecode.push_back(static_cast<uint16_t>(jump_pos));
            bytecode.push_back(bytecode.size() + 1);
            break;
        }
        case cppre::detail::QuantifierType::Optional:
        case cppre::detail::QuantifierType::Star: {
            const size_t split_pos = bytecode.size();
            bytecode.push_back(static_cast<uint16_t>(InstructionType::Split));
            bytecode.push_back(split_pos + 3);
            bytecode.push_back(-1);

            from_ast(node.node);

            if (node.repType == QuantifierType::Star) {
                bytecode.push_back(
                    static_cast<uint16_t>(InstructionType::Jump));
                bytecode.push_back(split_pos);
            }
            bytecode[split_pos + 2] = bytecode.size();
            break;
        }
    }
}

auto VM::make_code(CharClassNode const& node) -> void {
    char_classes.push_back(node.in_class);
    bytecode.push_back(
        static_cast<uint16_t>(node.inverted ? InstructionType::InvertedCharClass
                                            : InstructionType::CharClass));
    bytecode.push_back(char_classes.size() - 1);
}

namespace {
auto inst2string(InstructionType type) -> std::string_view {
    switch (type) {
        case cppre::detail::InstructionType::Split:
            return "Split";
        case cppre::detail::InstructionType::Save:
            return "Save";
        case cppre::detail::InstructionType::Any:
            return "Any";
        case cppre::detail::InstructionType::Jump:
            return "Jump";
        case cppre::detail::InstructionType::Match:
            return "Match";
        case cppre::detail::InstructionType::String:
            return "String";
        case cppre::detail::InstructionType::Anchor:
            return "Anchor";
        case cppre::detail::InstructionType::CharClass:
            return "CharClass";
        case cppre::detail::InstructionType::InvertedCharClass:
            return "InvertedCharClass";
    }
    // unreachable
    return "";
}

auto copy_saved(Thread::SharedSavedArray const& saved)
    -> Thread::SharedSavedArray {
    return std::make_shared<Thread::SavedArray>(*saved);
}

auto update_saved(Thread::SharedSavedArray&& saved, size_t idx, int n)
    -> Thread::SharedSavedArray {
    if (saved.use_count() == 1) {
        (*saved)[idx] = n;
        return std::move(saved);
    }

    auto new_saved = copy_saved(saved);
    (*new_saved)[idx] = n;
    return new_saved;
}
}  // namespace

Thread::Thread(size_t ngroups, size_t pc, size_t sp)
    : pc(pc),
      sp(sp),
      saved(std::make_shared<Thread::SavedArray>(2 * ngroups, -1)) {}
Thread::Thread(size_t pc, size_t sp, SharedSavedArray const& old_saved)
    : pc(pc), sp(sp), saved(old_saved) {}
Thread::Thread(size_t pc, size_t sp, SharedSavedArray&& old_saved)
    : pc(pc), sp(sp), saved(std::move(old_saved)) {}
Thread::Thread(size_t pc, SharedSavedArray const& old_saved)
    : pc(pc), saved(old_saved) {}
Thread::Thread(size_t pc, SharedSavedArray&& old_saved)
    : pc(pc), saved(std::move(old_saved)) {}

VM::VM(ASTNodePtr const& ast) {
    bytecode.push_back(static_cast<uint16_t>(InstructionType::Split));
    bytecode.push_back(6);
    bytecode.push_back(3);
    bytecode.push_back(static_cast<uint16_t>(InstructionType::Any));
    bytecode.push_back(static_cast<uint16_t>(InstructionType::Jump));
    bytecode.push_back(0);
    from_ast(ast);
    bytecode.push_back(static_cast<uint16_t>(InstructionType::Match));
    visited.resize(bytecode.size(), false);
}

auto VM::add_thread(ThreadList& tlist, Thread&& new_thread,
                    std::string_view const& str) -> void {
    size_t pc = new_thread.pc;

    if (visited[pc]) {
        return;
    }

    visited[pc] = true;

    if (new_thread.sp > str.length())
        return;

    switch (static_cast<InstructionType>(bytecode[pc])) {
        case cppre::detail::InstructionType::Split: {
            add_thread(
                tlist,
                Thread(bytecode[pc + 1], new_thread.sp, new_thread.saved), str);
            new_thread.pc = bytecode[pc + 2];
            add_thread(tlist,
                       Thread(bytecode[pc + 2], new_thread.sp,
                              std::move(new_thread.saved)),
                       str);
            break;
        }
        case cppre::detail::InstructionType::Jump:
            new_thread.pc = bytecode[pc + 1];
            add_thread(tlist, std::move(new_thread), str);
            break;
        case cppre::detail::InstructionType::Anchor: {
            bool assertion = false;
            switch (bytecode[pc + 1]) {
                case 'A':
                    assertion = new_thread.sp == 0;
                    break;
                case '^':
                    assertion =
                        new_thread.sp == 0 || str[new_thread.sp - 1] == '\n';
                    break;
                case 'Z':
                    assertion = new_thread.sp == str.length();
                    break;
                case '$':
                    assertion = new_thread.sp == str.length() ||
                                str[new_thread.sp] == '\n';
                    break;
                case 'b':
                    assertion = new_thread.sp == 0 ||
                                new_thread.sp == str.length() ||
                                std::isspace(str[new_thread.sp - 1]) ||
                                std::isspace(str[new_thread.sp]);
                    break;
                case 'B':
                    assertion =
                        !(new_thread.sp == 0 || new_thread.sp == str.length() ||
                          std::isspace(str[new_thread.sp - 1]) ||
                          std::isspace(str[new_thread.sp]));
                    break;
            }

            if (assertion) {
                new_thread.pc += 2;
                add_thread(tlist, std::move(new_thread), str);
            }
            break;
        }
        case cppre::detail::InstructionType::Save:
            add_thread(tlist,
                       Thread(new_thread.pc + 2, new_thread.sp,
                              update_saved(std::move(new_thread.saved),
                                           bytecode[pc + 1], new_thread.sp)),
                       str);
            break;
        default:
            tlist.push_back(std::move(new_thread));
            return;
    }
}

auto VM::run_thread(ThreadList& tlist, Thread const& thread,
                    std::string_view const& given) -> bool {
    size_t pc = thread.pc;
    switch (static_cast<InstructionType>(bytecode[pc])) {
        case InstructionType::String: {
            std::string_view const& str = strings[bytecode[pc + 1]];
            if (given.length() > thread.sp &&
                given.length() - thread.sp >= str.length() &&
                given.substr(thread.sp, str.length()) == str)
                add_thread(
                    tlist,
                    Thread(pc + 2, thread.sp + str.length(), thread.saved),
                    given);
            break;
        }

        case InstructionType::Any:
            if (thread.sp < given.length())
                add_thread(tlist, Thread(pc + 1, thread.sp + 1, thread.saved),
                           given);
            break;

        case cppre::detail::InstructionType::CharClass:
        case cppre::detail::InstructionType::InvertedCharClass: {
            const size_t cidx = bytecode[pc + 1];
            const InstructionType inst =
                static_cast<InstructionType>(bytecode[pc]);
            if (thread.sp < given.length() &&
                char_classes[cidx][given[thread.sp]] ==
                    (inst == InstructionType::CharClass))
                add_thread(tlist, Thread(pc + 2, thread.sp + 1, thread.saved),
                           given);
            break;
        }

        case InstructionType::Match:
            return true;

        default:
            assert(false);
    }
    return false;
}

auto VM::print_threadlist(ThreadList const& tl) const -> void {
    std::cout << "ThreadList:\n";
    for (auto const& t : tl) {
        std::cout << "    - pc: ";
        print_inst(t.pc);
        std::cout << "\n      sp: " << t.sp << "\n";
    }
    std::cout << "---------------\n";
}

auto VM::print_bytecode() const -> void {
    for (size_t i = 0; i < bytecode.size(); ++i) {
        std::cout << std::setw(4) << i;
    }
    std::cout << "\n";
    for (auto b : bytecode) {
        std::cout << std::setw(4) << std::hex << b;
    }
    std::cout << std::dec;
    std::cout << "\n";
}

auto VM::run_vm(std::string_view const& str, int pc_offset)
    -> std::optional<Thread::SavedArray> {
    ThreadList clist;
    ThreadList nlist;
    clist.reserve(bytecode.size());
    nlist.reserve(bytecode.size());

    std::optional<std::vector<int>> saved;

    add_thread(clist, Thread(ngroups, pc_offset, 0), str);
    do {
        std::fill(visited.begin(), visited.end(), false);

        if (clist.empty())
            break;
        for (auto const& thread : clist) {
            if (run_thread(nlist, thread, str)) {
                if (!saved) {
                    saved = std::make_optional(std::move(*thread.saved));
                } else if ((*saved)[0] == (*thread.saved)[0] &&
                           (*thread.saved)[1] > (*saved)[0]) {
                    saved = std::make_optional(std::move(*thread.saved));
                }
            }
        }
        std::swap(clist, nlist);
        nlist.clear();
    } while (true);

    return saved;
}

auto VM::print_inst(int i) const -> int {
    auto inst = static_cast<InstructionType>(bytecode[i]);
    std::cout << std::setw(3) << i << " | " << std::setw(12)
              << inst2string(inst);
    switch (inst) {
        case cppre::detail::InstructionType::Split:
            std::cout << bytecode[i + 1] << ", " << bytecode[i + 2];
            return i + 3;
        case cppre::detail::InstructionType::Jump:
        case cppre::detail::InstructionType::Save:
            std::cout << bytecode[i + 1];
            return i + 2;
        case cppre::detail::InstructionType::String: {
            const size_t sidx = bytecode[i + 1];
            std::cout << sidx << " (" << MAGENTA << "\"" << strings[sidx]
                      << "\"" << RESET << ")";
            return i + 2;
        }
        case cppre::detail::InstructionType::CharClass:
        case cppre::detail::InstructionType::InvertedCharClass: {
            const size_t cidx = bytecode[i + 1];
            std::cout << cidx;
            CharClassNode cc(inst == InstructionType::InvertedCharClass);
            cc.in_class = char_classes[cidx];
            std::cout << " " << cc.print_node(0);
            return i + 2;
        }
        case cppre::detail::InstructionType::Anchor:
            std::cout << static_cast<char>(bytecode[i + 1]);
            return i + 2;
        default:
            return i + 1;
    }
}

auto VM::print_code(size_t offset) const -> void {
    std::cout << std::left;
    for (size_t i = offset; i < bytecode.size();) {
        i = print_inst(i);
        std::cout << '\n';
    }
    std::cout << std::endl;
}

// 1200

}  // namespace detail
}  // namespace cppre