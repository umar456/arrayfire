/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once

#include <af/traits.hpp>
namespace af {

template<>
struct dtype_traits<std::complex<float> > {
    enum { af_type = c32 };
};

template<>
struct dtype_traits<std::complex<double> > {
    enum { af_type = c64 };
};

}

using af::dtype_traits;
