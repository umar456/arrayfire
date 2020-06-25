/*******************************************************
 * Copyright (c) 2018, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once

#include <Array.hpp>
#include <backend.hpp>
#include <common/defines.hpp>
#include <common/jit/Node.hpp>

#include <array>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

namespace common {

class NaryNode : public Node {
   private:
    int m_num_children;
    int m_op;
    std::string m_op_str;

   public:
    NaryNode(const af::dtype type, const char *op_str, const int num_children,
             const std::array<common::Node_ptr, Node::kMaxChildren> &&children,
             const int op, const int height)
        : common::Node(
              type, height,
              std::forward<
                  const std::array<common::Node_ptr, Node::kMaxChildren>>(
                  children))
        , m_num_children(num_children)
        , m_op(op)
        , m_op_str(op_str) {
        static_assert(std::is_nothrow_move_assignable<NaryNode>::value,
                      "NaryNode is not move assignable");
        static_assert(std::is_nothrow_move_constructible<NaryNode>::value,
                      "NaryNode is not move constructible");
    }

    NaryNode(NaryNode &&other) = default;

    NaryNode(const NaryNode &other) = default;

    /// Default copy assignment operator
    NaryNode &operator=(const NaryNode &node) = default;

    /// Default move assignment operator
    NaryNode &operator=(NaryNode &&node) noexcept = default;

    void swap(NaryNode &other) noexcept {
        using std::swap;
        Node::swap(other);
        swap(m_num_children, other.m_num_children);
        swap(m_op, other.m_op);
        swap(m_op_str, other.m_op_str);
    }

    void genKerName(std::stringstream &kerStream,
                    const common::Node_ids &ids) const final {
        // Make the dec representation of enum part of the Kernel name
        kerStream << "_" << std::setw(3) << std::setfill('0') << std::dec
                  << m_op;
        for (int i = 0; i < m_num_children; i++) {
            kerStream << std::setw(3) << std::setfill('0') << std::dec
                      << ids.child_ids[i];
        }
        kerStream << std::setw(3) << std::setfill('0') << std::dec << ids.id
                  << std::dec;
    }

    void genFuncs(std::stringstream &kerStream,
                  const common::Node_ids &ids) const final {
        kerStream << getTypeStr() << " val" << ids.id << " = " << m_op_str
                  << "(";
        for (int i = 0; i < m_num_children; i++) {
            if (i > 0) kerStream << ", ";
            kerStream << "val" << ids.child_ids[i];
        }
        kerStream << ");\n";
    }
};

template<typename Ti, int N, typename FUNC>
common::Node_ptr createNaryNode(
    const af::dim4 &odims, FUNC createNode,
    std::array<const detail::Array<Ti> *, N> &&children) {
    std::array<common::Node_ptr, N> childNodes;
    for (int i = 0; i < N; i++) { childNodes[i] = children[i]->getNode(); }

    common::Node_ptr ptr = createNode(childNodes);

    switch (static_cast<kJITHeuristics>(
        detail::passesJitHeuristics<Ti>(ptr.get()))) {
        case kJITHeuristics::Pass: {
            return ptr;
        }
        case kJITHeuristics::TreeHeight:
        case kJITHeuristics::KernelParameterSize: {
            int max_height_index = 0;
            int max_height       = 0;
            for (int i = 0; i < N; i++) {
                if (max_height < childNodes[i]->getHeight()) {
                    max_height_index = i;
                    max_height       = childNodes[i]->getHeight();
                }
            }

            children[max_height_index]->eval();
            return createNaryNode<Ti, N>(odims, createNode, move(children));
        }
        case kJITHeuristics::MemoryPressure: {
            for (auto &c : children) { c->eval(); }  // TODO: use evalMultiple()
            return ptr;
        }
    }
    return ptr;
}
}  // namespace common
