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
#include <string_view>
#include <utility>

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
    }
}

auto VM::make_code(WildcardNode const&) -> void {
    bytecode.push_back(static_cast<uint16_t>(InstructionType::Any));
    proglen++;
}

auto VM::make_code(LiteralNode const& node) -> void {
    proglen++;
    bytecode.push_back(static_cast<uint16_t>(InstructionType::String));
    bytecode.push_back(static_cast<uint16_t>(strings.size()));
    strings.push_back(std::string_view(node.value));
}

auto VM::make_code(GroupNode const& node) -> void {
    if (node.group_id != -1) {
        proglen++;
        bytecode.push_back(static_cast<uint16_t>(InstructionType::Save));
        bytecode.push_back(2 * node.group_id);
        ngroups = node.group_id + 1;
    }
    from_ast(node.node);
    if (node.group_id != -1) {
        proglen++;
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
    proglen += 2;
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
            proglen++;
            break;
        }
        case cppre::detail::QuantifierType::Optional:
        case cppre::detail::QuantifierType::Star: {
            const size_t split_pos = bytecode.size();
            bytecode.push_back(static_cast<uint16_t>(InstructionType::Split));
            bytecode.push_back(split_pos + 3);
            bytecode.push_back(-1);
            proglen++;

            from_ast(node.node);

            if (node.repType == QuantifierType::Star) {
                bytecode.push_back(
                    static_cast<uint16_t>(InstructionType::Jump));
                bytecode.push_back(split_pos);
                proglen++;
            }
            bytecode[split_pos + 2] = bytecode.size();
            break;
        }
    }
}

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
    }
}

VM::VM(ASTNodePtr const& ast) {
    from_ast(ast);
    bytecode.push_back(static_cast<uint16_t>(InstructionType::Match));
    proglen++;
}

auto VM::add_thread(ThreadList& tlist, Thread&& new_thread) -> void {
    size_t pc = new_thread.pc;
    if (0xf000 & bytecode[pc]) {
        return;
    }

    bytecode[pc] |= 0xf000;
    marked_insts.insert(pc);
    switch (static_cast<InstructionType>(bytecode[pc] & 0xff)) {
        case cppre::detail::InstructionType::Split: {
            add_thread(tlist, Thread(bytecode[pc + 1], new_thread.sp,
                                     new_thread.saved));
            new_thread.pc = bytecode[pc + 2];
            add_thread(tlist, std::move(new_thread));
            break;
        }
        case cppre::detail::InstructionType::Jump:
            new_thread.pc = bytecode[pc + 1];
            add_thread(tlist, std::move(new_thread));
            break;
        case cppre::detail::InstructionType::Save:
            new_thread.saved[bytecode[pc + 1]] = new_thread.sp;
            new_thread.pc += 2;
            break;
        default:
            tlist.push_back(std::move(new_thread));
            return;
    }
    bytecode[pc] &= 0xff;
    marked_insts.erase(pc);
}

auto VM::run_thread(ThreadList& tlist, Thread const& thread,
                    std::string_view const& given) -> bool {
    size_t pc = thread.pc;
    bytecode[pc] &= 0xff;
    switch (static_cast<InstructionType>(bytecode[pc])) {
        case InstructionType::String: {
            std::string_view const& str = strings[bytecode[pc + 1]];
            if (given.length() > thread.sp &&
                given.length() - thread.sp >= str.length() &&
                given.substr(thread.sp, str.length()) == str)
                add_thread(tlist, Thread(pc + 2, thread.sp + str.length(),
                                         thread.saved));
            break;
        }

        case InstructionType::Any:
            if (thread.sp < given.length())
                add_thread(tlist, Thread(pc + 1, thread.sp + 1, thread.saved));
            break;

        case InstructionType::Match:
            return true;

        default:
            assert(false);
    }
    return false;
}

auto print_threadlist(ThreadList const& tl) {
    std::cout << "ThreadList:\n";
    for (auto const& t : tl) {
        std::cout << "    pc: " << t.pc << " sp: " << t.sp << "\n";
    }
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

auto VM::run_vm(std::string_view const& str) -> bool {
    ThreadList clist;
    ThreadList nlist;
    clist.reserve(proglen);
    nlist.reserve(proglen);

    add_thread(clist, Thread(ngroups));
    do {
        std::for_each(marked_insts.cbegin(), marked_insts.cend(),
                      [this](size_t pc) { bytecode[pc] &= 0xff; });
        marked_insts.clear();

        if (clist.empty())
            return false;
        for (auto const& thread : clist) {
            if (run_thread(nlist, thread, str))
                return true;
        }
        std::swap(clist, nlist);
        nlist.clear();
    } while (true);

    return false;
}

auto VM::print_code() const -> void {
    std::cout << std::left;
    for (size_t i = 0; i < bytecode.size();) {
        auto inst = static_cast<InstructionType>(bytecode[i]);
        std::cout << std::setw(3) << i << " | " << std::setw(12)
                  << inst2string(inst);
        switch (inst) {
            case cppre::detail::InstructionType::Split:
                std::cout << bytecode[i + 1] << ", " << bytecode[i + 2];
                i += 3;
                break;
            case cppre::detail::InstructionType::Jump:
            case cppre::detail::InstructionType::Save:
                std::cout << bytecode[i + 1];
                i += 2;
                break;
            case cppre::detail::InstructionType::String: {
                const size_t sidx = bytecode[i + 1];
                std::cout << sidx << " (" << MAGENTA << "\"" << strings[sidx]
                          << "\"" << RESET << ")";
                i += 2;
                break;
            }
            default:
                i++;
        }
        std::cout << std::endl;
    }
}

// 1200

}  // namespace detail
}  // namespace cppre