/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once
#include "Node.hpp"
#include <math.hpp>
#include <types.hpp>

#include <spdlog/fmt/ostr.h>

namespace opencl
{

namespace JIT
{

    template <typename T>
    class ScalarNode : public Node
    {
    private:
        const T m_val;

    public:

        ScalarNode(T val)
            : Node(dtype_traits<T>::getName(), shortname<T>(false), 0, {}),
              m_val(val)
        {
        }

        void genParams(std::stringstream &kerStream, int id, bool is_linear) const final
        {
            fmt::print(kerStream, "{0} scalar{1},\n", m_type_str, id);
        }

        int setArgs(cl::Kernel &ker, int id, bool is_linear) const final
        {
            ker.setArg(id, m_val);
            return id + 1;
        }

        void genFuncs(std::stringstream &kerStream, Node_ids ids) const final
        {
            fmt::print(kerStream, "{0} val{1} = scalar{1};\n", m_type_str, ids.id);
        }

        // Return the info for the params and the size of the buffers
        virtual short getParamBytes() const final { return static_cast<short>(sizeof(T)); }
    };

}

}
