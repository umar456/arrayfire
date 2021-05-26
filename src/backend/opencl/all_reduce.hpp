/*******************************************************
 * Copyright (c) 2021, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once
#include <Array.hpp>
#include <vector>

namespace opencl {

template<af_op_t op, typename Ti>
void all_reduce(std::vector<Array<Ti> *> in);

}  // namespace opencl
