/*******************************************************
 * Copyright (c) 2018, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once
#include "BufferNode.hpp"
#include "Node.hpp"
#include <memory>

namespace cuda
{

namespace JIT
{
    template<typename T>
    class ShiftNode : public Node
    {
    private:

        std::shared_ptr<BufferNode<T>> m_buffer_node;
        const std::array<int, 4> m_shifts;

    public:

        ShiftNode(const char *type_str,
                  const char *name_str,
                  std::shared_ptr<BufferNode<T>> buffer_node,
                  const std::array<int, 4> shifts)
            : Node(type_str, name_str, 0, {}),
              m_buffer_node(buffer_node),
              m_shifts(shifts)
        {
        }

        void setData(Param<T> param, std::shared_ptr<T> data, const unsigned bytes, bool is_linear)
        {
            m_buffer_node->setData(param, data, bytes, is_linear);
        }

        bool isLinear(dim_t dims[4]) const final
        {
            return false;
        }

        void genParams(std::stringstream &kerStream, int id, bool is_linear) const final
        {
            m_buffer_node->genParams(kerStream, id, is_linear);
            fmt::print(kerStream,
                       "int shift{0}_0,\n"
                       "int shift{0}_1,\n"
                       "int shift{0}_2,\n"
                       "int shift{0}_3,\n", id);
        }

        void setArgs(std::vector<void *> &args, bool is_linear) const final
        {
            auto node_ptr = m_buffer_node.get();
            dynamic_cast<BufferNode<T> *>(node_ptr)->setArgs(args, is_linear);
            for (int i = 0; i < 4; i++) {
                const int &d = m_shifts[i];
                args.push_back((void *)&d);
            }
        }

        void genOffsets(std::stringstream &kerStream, int id, bool is_linear) const final
        {
            fmt::print(kerStream, R"Shift(
                int sh_id_{0}_0 = __circular_mod(id0 + shift{0}_0, in{0}.dims[0]);
                int sh_id_{0}_1 = __circular_mod(id1 + shift{0}_1, in{0}.dims[1]);
                int sh_id_{0}_2 = __circular_mod(id2 + shift{0}_2, in{0}.dims[2]);
                int sh_id_{0}_3 = __circular_mod(id3 + shift{0}_3, in{0}.dims[3]);
                int idx{0} =  (sh_id_{0}_3 < in{0}.dims[3]) * in{0}.strides[3] * sh_id_{0}_3;
                    idx{0} += (sh_id_{0}_2 < in{0}.dims[2]) * in{0}.strides[2] * sh_id_{0}_2;
                    idx{0} += (sh_id_{0}_1 < in{0}.dims[1]) * in{0}.strides[1] * sh_id_{0}_1;
                    idx{0} += (sh_id_{0}_0 < in{0}.dims[0]) * sh_id_{0}_0;
                {1} *in{0}_ptr = in{0}.ptr;
                )Shift", id, m_type_str);
        }

        void genFuncs(std::stringstream &kerStream, Node_ids ids) const final
        {
            fmt::print(kerStream, "{0} val{1} = in{1}_ptr[idx{1}];\n",
                       m_type_str, ids.id);
        }

        void getInfo(unsigned &len, unsigned &buf_count, unsigned &bytes) const final
        {
            auto node_ptr = m_buffer_node.get();
            dynamic_cast<BufferNode<T> *>(node_ptr)->getInfo(len, buf_count, bytes);
        }
    };
}

}
