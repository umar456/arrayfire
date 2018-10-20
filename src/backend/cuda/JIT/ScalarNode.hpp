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

#pragma push
#pragma diag_suppress = code_is_unreachable
#include <spdlog/fmt/ostr.h>
#pragma pop

namespace cuda
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
            : Node(getFullName<T>(), shortname<T>(false), 0, {}),
              m_val(val)
        {
        }

        void genParams(std::stringstream &kerStream, int id, bool is_linear) const final
        {
            fmt::print(kerStream, "{0} scalar{1},\n", m_type_str, id);
        }

        void setArgs(std::vector<void *> &args, bool is_linear) const final
        {
            args.push_back((void *)&m_val);
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
