/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#include "reduce_impl.hpp"

namespace opencl
{

#define INSTANTIATE(Op, Ti, To)                                             \
  template Array<To> reduce<Op, Ti, To>(const Array<Ti> &in, const int dim, \
                                        bool change_nan, double nanval);    \
  template To reduce_all<Op, Ti, To>(const Array<Ti> &in,                   \
                                     bool change_nan, double nanval);

//anytrue
INSTANTIATE(af_or_t, float  , char)
INSTANTIATE(af_or_t, double , char)
INSTANTIATE(af_or_t, cfloat , char)
INSTANTIATE(af_or_t, cdouble, char)
INSTANTIATE(af_or_t, int    , char)
INSTANTIATE(af_or_t, uint   , char)
INSTANTIATE(af_or_t, intl   , char)
INSTANTIATE(af_or_t, uintl  , char)
INSTANTIATE(af_or_t, char   , char)
INSTANTIATE(af_or_t, uchar  , char)
INSTANTIATE(af_or_t, short  , char)
INSTANTIATE(af_or_t, ushort , char)

#undef INSTANTIATE
}
