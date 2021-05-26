/*******************************************************
 * Copyright (c) 2021, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#include <Array.hpp>
#include <all_reduce.hpp>
#include <platform.hpp>

#include <iterator>
#include <vector>

namespace opencl {
template<af_op_t op, typename Ti>
void all_reduce(std::vector<Array<Ti> *> in) {
    OPENCL_NOT_SUPPORTED("all_reduce not supported on OpenCL");
}

template void all_reduce<af_add_t, float>(std::vector<Array<float> *> in);
}  // namespace opencl
