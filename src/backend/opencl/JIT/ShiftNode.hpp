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
#include <spdlog/fmt/ostr.h>

#include <iomanip>
#include <memory>

namespace opencl
{

namespace JIT
{
    class ShiftNode : public Node
    {
    private:

        std::shared_ptr<BufferNode> m_buffer_node;
        const std::array<int, 4> m_shifts;

    public:

        ShiftNode(const char *type_str,
                  const char *name_str,
                  std::shared_ptr<BufferNode> buffer_node,
                  const std::array<int, 4> shifts)
            : Node(type_str, name_str, 0, {}),
              m_buffer_node(buffer_node),
              m_shifts(shifts)
        {
        }

        void setData(KParam info, std::shared_ptr<cl::Buffer> data, const unsigned bytes, bool is_linear)
        {
            m_buffer_node->setData(info, data, bytes, is_linear);
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

        int setArgs(cl::Kernel &ker, int id, bool is_linear) const final
        {
            int curr_id = m_buffer_node->setArgs(ker, id, is_linear);
            for (int i = 0; i < 4; i++) {
                ker.setArg(curr_id + i, m_shifts[i]);
            }
            return curr_id + 4;
        }

        void genOffsets(std::stringstream &kerStream, int id, bool is_linear) const final
        {
             fmt::print(kerStream, R"Shift(
             int sh_id_{0}_0 = __circular_mod(id0 + shift{0}_0, iInfo{0}.dims[0]);
             int sh_id_{0}_1 = __circular_mod(id1 + shift{0}_1, iInfo{0}.dims[1]);
             int sh_id_{0}_2 = __circular_mod(id2 + shift{0}_2, iInfo{0}.dims[2]);
             int sh_id_{0}_3 = __circular_mod(id3 + shift{0}_3, iInfo{0}.dims[3]);
             int idx{0} = (sh_id_{0}_3 < iInfo{0}.dims[3]) * iInfo{0}.strides[3] * sh_id_{0}_3;
             idx{0} += (sh_id_{0}_2 < iInfo{0}.dims[2]) * iInfo{0}.strides[2] * sh_id_{0}_2;
             idx{0} += (sh_id_{0}_1 < iInfo{0}.dims[1]) * iInfo{0}.strides[1] * sh_id_{0}_1;
             idx{0} += (sh_id_{0}_0 < iInfo{0}.dims[0]) * sh_id_{0}_0 + iInfo{0}.offset;)Shift" "\n", id);
        }

        void genFuncs(std::stringstream &kerStream, Node_ids ids) const final
        {
            fmt::print(kerStream, "{0} val{1} = in{1}[idx{1}];\n",
                       m_type_str, ids.id);
        }

        void getInfo(unsigned &len, unsigned &buf_count, unsigned &bytes) const final
        {
            m_buffer_node->getInfo(len, buf_count, bytes);
        }
    };
}

}
