
#pragma once
#include <common/jit/Node.hpp>
#include <common/jit/ScalarNode.hpp>

namespace common {

class INodeVisitor {
   public:
    virtual void operator()(ScalarNode<float>& node)  = 0;
    virtual void operator()(ScalarNode<double>& node) = 0;
    virtual void operator()(ScalarNode<int>& node)    = 0;
    virtual void operator()(Node& node)               = 0;
    // virtual void operator()(NaryNode& node) = 0;
    // virtual void operator()(ShiftNode& node) = 0;
};

}  // namespace common
