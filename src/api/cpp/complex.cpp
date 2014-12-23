/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#include <af/complex.h>

namespace af
{
    cfloat::cfloat()
    {
        cval = {0, 0};
    }

    cfloat::cfloat(const float &real, const float &imag)
    {
        cval = {real, imag};
    }

    float cfloat::real() const
    {
        return cval.x;
    }

    float cfloat::imag() const
    {
        return cval.y;
    }

    cfloat cfloat::conj() const
    {
        return cfloat(cval.x, -cval.y);
    }

    cfloat cfloat::operator+(const cfloat &val) const
    {
        float real = val.real() + this->real();
        float imag = val.imag() + this->imag();
        return cfloat(real, imag);
    }

    cfloat cfloat::operator+(const float &val) const
    {
        float real = val + this->real();
        float imag = this->imag();
        return cfloat(real, imag);
    }

    cfloat cfloat::operator-(const cfloat &val) const
    {
        float real = this->real() - val.real();
        float imag = this->imag() - val.imag();
        return cfloat(real, imag);
    }

    cfloat cfloat::operator-(const float &val) const
    {
        float real = this->real() - val;
        float imag = this->imag();
        return cfloat(real, imag);
    }


    cfloat operator+(const float &lhs, const cfloat &rhs)
    {
        float real = lhs + rhs.real();
        float imag = rhs.imag();
        return cfloat(real, imag);
    }

    cfloat operator-(const float &lhs, const cfloat &rhs)
    {
        float real = lhs - rhs.real();
        float imag = -rhs.imag();
        return cfloat(real, imag);
    }

#ifndef NO_CPP_11
    cfloat::operator std_cfloat() const
    {
        return std_cfloat(real(), imag());
    }

    cfloat cfloat::operator=(const std_cfloat &in) const
    {
        return cfloat(in.real(), in.imag());
    }
#endif

    cdouble::cdouble()
    {
        cval = {0, 0};
    }

    cdouble::cdouble(const double &real, const double &imag)
    {
        cval = {real, imag};
    }

    double cdouble::real() const
    {
        return cval.x;
    }

    double cdouble::imag() const
    {
        return cval.y;
    }

    cdouble cdouble::conj() const
    {
        return cdouble(cval.x, -cval.y);
    }

    cdouble cdouble::operator+(const cdouble &val) const
    {
        double real = val.real() + this->real();
        double imag = val.imag() + this->imag();
        return cdouble(real, imag);
    }

    cdouble cdouble::operator+(const double &val) const
    {
        double real = val + this->real();
        double imag = this->imag();
        return cdouble(real, imag);
    }

    cdouble cdouble::operator-(const cdouble &val) const
    {
        double real = this->real() - val.real();
        double imag = this->imag() - val.imag();
        return cdouble(real, imag);
    }

    cdouble cdouble::operator-(const double &val) const
    {
        double real = this->real() - val;
        double imag = this->imag();
        return cdouble(real, imag);
    }


    cdouble operator+(const double &lhs, const cdouble &rhs)
    {
        double real = lhs + rhs.real();
        double imag = rhs.imag();
        return cdouble(real, imag);
    }

    cdouble operator-(const double &lhs, const cdouble &rhs)
    {
        double real = lhs - rhs.real();
        double imag = -rhs.imag();
        return cdouble(real, imag);
    }

#ifndef NO_CPP_11
    cdouble::operator std_cdouble() const
    {
        return std_cdouble(real(), imag());
    }

    cdouble cdouble::operator=(const std_cdouble &in) const
    {
        return cdouble(in.real(), in.imag());
    }
#endif

}
