
#include <common/jit/Node.hpp>
#include <common/jit/NodeVisitor.hpp>
#include <common/jit/ScalarNode.hpp>
#include <cstdio>

namespace common {
// template<typename T>
// void ScalarNode<T>::visit(INodeVisitor& v) {
//     printf("ha\n");
//     v(*this);
// }
//
// template<>
// void ScalarNode<float>::visit(INodeVisitor& v);

void Node::visit(INodeVisitor& v) {
    Node& t = *this;
    v(t);
    printf("ha no\n");
}

}  // namespace common
