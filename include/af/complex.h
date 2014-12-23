/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/
#pragma once
#ifndef NO_CPP_11
#include <complex>
typedef std::complex<float> std_cfloat;
typedef std::complex<double> std_cdouble;
#endif
#include <af/defines.h>

typedef af_float2 af_cfloat;
typedef af_double2 af_cdouble;

#ifdef __cplusplus
namespace af
{
    class AFAPI cfloat
    {
    private:
        af_cfloat cval;

    public:
        cfloat();
        cfloat(const float &real, const float &imag=0);
        AFAPI float real() const;
        AFAPI float imag() const;
        AFAPI cfloat conj() const;
        AFAPI cfloat operator+(const float  &val) const;
        AFAPI cfloat operator+(const cfloat &val) const;
        AFAPI cfloat operator-(const float  &val) const;
        AFAPI cfloat operator-(const cfloat &val) const;
        AFAPI friend cfloat operator+(const float  &lhs, const cfloat &rhs);
        AFAPI friend cfloat operator-(const float  &lhs, const cfloat &rhs);

#ifndef NO_CPP_11
        AFAPI operator std_cfloat() const;
        AFAPI cfloat operator=(const std_cfloat &rhs) const;
#endif
    };

    AFAPI cfloat operator+(const float &lhs, const cfloat &rhs);
    AFAPI cfloat operator-(const float &lhs, const cfloat &rhs);

    class AFAPI cdouble
    {
    private:
        af_cdouble cval;

    public:
        cdouble();
        cdouble(const double &real, const double &imag=0);
        AFAPI double real() const;
        AFAPI double imag() const;
        AFAPI cdouble conj() const;
        AFAPI cdouble operator+(const double  &val) const;
        AFAPI cdouble operator+(const cdouble &val) const;
        AFAPI cdouble operator-(const double  &val) const;
        AFAPI cdouble operator-(const cdouble &val) const;
        AFAPI friend cdouble operator+(const double &lhs, const cdouble &rhs);
        AFAPI friend cdouble operator-(const double &lhs, const cdouble &rhs);

#ifndef NO_CPP_11
        AFAPI operator std_cdouble() const;
        AFAPI cdouble operator=(const std_cdouble &rhs) const;
#endif
    };

    AFAPI cdouble operator+(const double &lhs, const cdouble &rhs);
    AFAPI cdouble operator-(const double &lhs, const cdouble &rhs);
}
#endif
