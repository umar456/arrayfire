/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once
#include "../Param.hpp"
#include "Node.hpp"
#include <mutex>

#pragma push
#pragma diag_suppress = code_is_unreachable
#include <spdlog/fmt/ostr.h>
#pragma pop

namespace cuda
{

namespace JIT
{


    template<typename T>
    class BufferNode : public Node
    {
    private:
        std::shared_ptr<T> m_data;
        Param<T> m_param;
        unsigned m_bytes;
        std::once_flag m_set_data_flag;
        bool m_linear_buffer;

    public:

        BufferNode(const char *type_str,
                   const char *name_str)
            : Node(type_str, name_str, 0, {})
        {
        }

        bool isBuffer() const final { return true; }

        void setData(Param<T> param, std::shared_ptr<T> data, const unsigned bytes, bool is_linear)
        {
            std::call_once(m_set_data_flag, [this, param, data, bytes, is_linear]() {
                    m_param = param;
                    m_data = data;
                    m_bytes = bytes;
                    m_linear_buffer = is_linear;
                });
        }

        bool isLinear(dim_t dims[4]) const final
        {
            bool same_dims = true;
            for (int i = 0; same_dims && i < 4; i++) {
                same_dims &= (dims[i] == m_param.dims[i]);
            }
            return m_linear_buffer && same_dims;
        }

        void genParams(std::stringstream &kerStream, int id, bool is_linear) const final
        {
            if (is_linear) {
                fmt::print(kerStream, "{0} *in{1}_ptr,\n", m_type_str, id);
            } else {
                fmt::print(kerStream, "Param<{0}> in{1},\n", m_type_str, id);
            }
        }

        void setArgs(std::vector<void *> &args, bool is_linear) const final
        {
            if (is_linear) {
                args.push_back((void *)&m_param.ptr);
            } else {
                args.push_back((void *)&m_param);
            }
        }

        void genOffsets(std::stringstream &kerStream, int id, bool is_linear) const final
        {
            if (is_linear) {
                fmt::print(kerStream, "int idx{0} = idx;\n", id);
            } else {
                fmt::print(kerStream,
                          "int idx{0} = "
                          "(id3 < in{0}.dims[3]) * in{0}.strides[3] * id3 + "
                          "(id2 < in{0}.dims[2]) * in{0}.strides[2] * id2 + "
                          "(id1 < in{0}.dims[1]) * in{0}.strides[1] * id1 + "
                          "(id0 < in{0}.dims[0]) * in{0}.strides[0] * id0;\n"
                           "{1} *in{0}_ptr = in{0}.ptr;\n", id, m_type_str);
            }
        }

        void genFuncs(std::stringstream &kerStream, Node_ids ids) const final
        {
            fmt::print(kerStream, "{0} val{1} = in{1}_ptr[idx{1}];\n",
                       m_type_str, ids.id);
        }

        void getInfo(unsigned &len, unsigned &buf_count, unsigned &bytes) const final
        {
            len++;
            buf_count++;
            bytes += m_bytes;
            return;
        }

        // Return the size of the size of the buffer node in bytes. Zero otherwise
        virtual size_t getBytes() const final {
            return m_bytes;
        }

        // Return the size of the parameter in bytes that will be passed to the
        // kernel
        virtual short getParamBytes() const final {
            return m_linear_buffer ? sizeof(T*) : sizeof(Param<T>);
        }
    };


}

}
