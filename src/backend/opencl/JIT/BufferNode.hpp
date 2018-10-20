/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once
#include <af/defines.h>
#include "../kernel/KParam.hpp"
#include "Node.hpp"

#include <spdlog/fmt/ostr.h>

#include <iomanip>
#include <memory>

namespace opencl
{

namespace JIT
{


    class BufferNode : public Node
    {
    private:
        std::shared_ptr<cl::Buffer> m_data;
        KParam m_info;
        unsigned m_bytes;
        std::once_flag m_set_data_flag;
        bool m_linear_buffer;

    public:

        BufferNode(const char *type_str,
                   const char *name_str)
            : Node(type_str, name_str, 0, {})
        {
        }

        bool isBuffer() const final {
            return true;
        }

        void setData(KParam info, std::shared_ptr<cl::Buffer> data, const unsigned bytes, bool is_linear)
        {
            std::call_once(m_set_data_flag, [this, info, data, bytes, is_linear]() {
                    m_info = info;
                    m_data = data;
                    m_bytes = bytes;
                    m_linear_buffer = is_linear;
                });
        }

        bool isLinear(dim_t dims[4]) const final
        {
            bool same_dims = true;
            for (int i = 0; same_dims && i < 4; i++) {
                same_dims &= (dims[i] == m_info.dims[i]);
            }
            return m_linear_buffer && same_dims;
        }

        void genParams(std::stringstream &kerStream, int id, bool is_linear) const final
        {
            if (is_linear) {
                fmt::print(kerStream,
                          "__global {0} *in{1}, dim_t iInfo{1}_offset,\n",
                          m_type_str, id);
            } else {
                fmt::print(kerStream,
                          "__global {0} *in{1}, KParam iInfo{1},\n",
                          m_type_str, id);
            }
        }

        int setArgs(cl::Kernel &ker, int id, bool is_linear) const final
        {
            ker.setArg(id + 0, *m_data);
            if (!is_linear) {
                ker.setArg(id + 1,  m_info);
            } else {
                ker.setArg(id + 1, m_info.offset);
            }
            return id + 2;
        }

        void genOffsets(std::stringstream &kerStream, int id, bool is_linear) const final
        {
            if (is_linear) {
                fmt::print(kerStream, "int idx{0} = idx + iInfo{0}_offset;\n", id);
            } else {
                fmt::print(kerStream,
                          "int idx{0} = "
                          "(id3 < iInfo{0}.dims[3]) * iInfo{0}.strides[3] * id3 + "
                          "(id2 < iInfo{0}.dims[2]) * iInfo{0}.strides[2] * id2 + "
                          "(id1 < iInfo{0}.dims[1]) * iInfo{0}.strides[1] * id1 + "
                          "(id0 < iInfo{0}.dims[0]) * iInfo{0}.strides[0] * id0 + "
                          "iInfo{0}.offset;\n", id);
            }
        }

        // Return the size of the parameter in bytes that will be passed to the
        // kernel
        virtual short getParamBytes() const final {
            return m_linear_buffer ? sizeof(void*) : sizeof(KParam);
        }

        void genFuncs(std::stringstream &kerStream, Node_ids ids) const final
        {
            fmt::print(kerStream, "{0} val{1} = in{1}[idx{1}];\n", m_type_str, ids.id);
        }

        void getInfo(unsigned &len, unsigned &buf_count, unsigned &bytes) const final
        {
            len++;
            buf_count++;
            bytes += m_bytes;
        }

        size_t getBytes() const final { return m_bytes; }
    };

}

}
