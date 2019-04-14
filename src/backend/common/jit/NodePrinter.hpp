/*******************************************************
 * Copyright (c) 2018, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once
#include <backend.hpp>
#include <common/jit/NaryNode.hpp>
#include <common/jit/Node.hpp>
#include <common/jit/NodeIterator.hpp>
#include <common/jit/NodeVisitor.hpp>

#include <cstdio>
#include <string>
#include <utility>
#include <vector>

using common::NaryNode;
using common::Node;
using common::NodeIterator;

using std::make_pair;
using std::string;
using std::tie;

namespace common {

const char* ops[] = {
    "af_add",

    "af_sub",     "af_mul",        "af_div",

    "af_and",     "af_or",         "af_eq",     "af_neq",       "af_lt",
    "af_le",      "af_gt",         "af_ge",

    "af_bitor",   "af_bitand",     "af_bitxor", "af_bitshiftl", "af_bitshiftr",

    "af_min",     "af_max",        "af_cplx2",  "af_atan2",     "af_pow",
    "af_hypot",

    "af_sin",     "af_cos",        "afan",      "af_asin",      "af_acos",
    "af_atan",

    "af_sinh",    "af_cosh",       "afanh",     "af_asinh",     "af_acosh",
    "af_atanh",

    "af_exp",     "af_expm1",      "af_erf",    "af_erfc",

    "af_log",     "af_log10",      "af_log1p",  "af_log2",

    "af_sqrt",    "af_cbrt",

    "af_abs",     "af_cast",       "af_cplx",   "af_real",      "af_imag",
    "af_conj",

    "af_floor",   "af_ceil",       "af_round",  "af_trunc",     "af_signbit",

    "af_rem",     "af_mod",

    "af_tgamma",  "af_lgamma",

    "af_notzero",

    "af_iszero",  "af_isinf",      "af_isnan",

    "af_sigmoid",

    "af_noop",

    "af_select",  "af_not_select",
};

string int_to_name(int val) {
    string out;
    do {
        out += static_cast<char>(val % 26 + 'A');
        val -= 25;
    } while (val > 0);
    return out;
}

class DotVisitor : public INodeVisitor {
    Node_map_t& id_map_;

   public:
    DotVisitor(Node_map_t& id_map) : id_map_(id_map) {}

    void operator()(ScalarNode<float>& sn) {
        printf("\"val%d\" [label = \"scalar%d\"]\n", id_map_[&sn],
               id_map_[&sn]);
    }
    void operator()(ScalarNode<double>& sn) {
        printf("\"val%d\" [label = \"scalar%d\"]\n", id_map_[&sn],
               id_map_[&sn]);
    }
    void operator()(ScalarNode<int>& sn) {
        printf("\"val%d\" [label = \"scalar%d\"]\n", id_map_[&sn],
               id_map_[&sn]);
    }
    void operator()(Node& sn) { printf("just a node\n"); }
};

void printNodes(Node* node) {
    // Use thread local to reuse the memory every time you are here.
    Node_map_t nodes;
    std::vector<Node*> full_nodes;
    std::vector<Node_ids> full_ids;
    std::vector<int> output_ids;
    int id = node->getNodesMap(nodes, full_nodes, full_ids);

    printf("digraph g {\n");
    printf("\t\"val%d\":f0 -> \"out%d_ptr\"\n", id, id);

    long buffer_count = 0;

    DotVisitor dv(nodes);
    for (const Node* n : full_nodes) { ((Node*)n)->visit(dv); }

    // map<Node *, string> map;
    // for (NodeIterator<> n(node); n != NodeIterator<>(); ++n) {
    // bool success = false;
    // std::map<Node *, string>::iterator it;
    // tie(it, success) =
    //    map.insert(make_pair(&(*n), int_to_name(buffer_count)));
    // if (success) { buffer_count++; }

    // printf("\t");
    // switch (n->getNodeType()) {
    //    case NodeType::Buffer:
    //        printf("\"val%d\" [shape=record, label=\"in%d_ptr\"]\n",
    //               nodes[it->first], nodes[it->first]);
    //        break;
    //    case NodeType::Nary: {
    //        NaryNode *narynode = static_cast<NaryNode *>(&(*n));
    //        auto children      = n->getChildren();

    //        std::map<Node *, string>::iterator it_child;
    //        for (int i = 0; i < narynode->getNumChildren(); i++) {
    //            tie(it_child, success) = map.insert(make_pair(
    //                children[i].get(), int_to_name(buffer_count)));
    //            if (success) { buffer_count++; }

    //            if (it_child->first->getNodeType() == NodeType::Nary) {
    //                printf("\"val%d\":f0 -> \"val%d\":f%d\n\t",
    //                       nodes[it_child->first], nodes[it->first], i + 1);
    //            } else {
    //                printf("\"val%d\" -> \"val%d\":f%d\n\t",
    //                       nodes[it_child->first], nodes[it->first], i + 1);
    //            }
    //        }

    //        printf("\"val%d\" [shape=record, label=\"<f0> %s ",
    //               nodes[it->first], ops[narynode->getOp()]);
    //        for (int i = 0; i < narynode->getNumChildren(); i++) {
    //            printf(" | <f%d> val%d", i + 1, nodes[children[i].get()]);
    //        }
    //        printf("\"]\n");
    //    } break;
    //    case NodeType::Scalar:
    //        if (!success)
    //            printf("\"val%d\" [label = \"scalar%d\"]\n",
    //                   nodes[it->first], nodes[it->first]);
    //        break;
    //}
    //}

    printf("}\n");
}

}  // namespace common
