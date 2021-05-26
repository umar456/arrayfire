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
#include <err_cpu.hpp>
#include <vector>

namespace cpu {
template<af_op_t op, typename Ti>
void all_reduce(std::vector<Array<Ti> *> in) {
    CPU_NOT_SUPPORTED("af::all_reduce not supported on cpu");
}

template void all_reduce<af_add_t, float>(std::vector<Array<float> *> in);
}  // namespace cpu
