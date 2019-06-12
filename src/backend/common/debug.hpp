
#pragma once

#include <Array.hpp>
#include <backend.hpp>
#include <types.hpp>
#include <af/dim4.hpp>

#include <transpose.hpp>

#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>

int get_stream_manipulation_index();

namespace {

struct print_state {
    int print_info;
    int width;
    int precision;

    // number of elements to print from the beginning
    af::dim4 num_elements_begin;

    // number of elements to print from the end
    af::dim4 num_elements_end;

    print_state(int pinfo = 0, af::dim4 begin = af::dim4(8, 8, 1, 1),
                af::dim4 end = af::dim4(2, 2, 0, 0))
        : print_info(pinfo)
        , width(-1)
        , precision(-1)
        , num_elements_begin(begin)
        , num_elements_end(end) {}
};

std::ostream &with_info(std::ostream &os) {
    print_state *&ptr =
        (print_state *&)os.pword(get_stream_manipulation_index());
    if (ptr) {
        ptr->print_info = 1;
    } else {
        ptr = new print_state(1);
    }
    return os;
}

struct print_extents {
    af::dim4 begin;
    af::dim4 end;
};

print_extents with_extents(af::dim4 b = {8, 8, 1, 1},
                           af::dim4 e = {2, 2, 0, 0}) {
    return {b, e};
}

std::ostream &operator<<(std::ostream &os, print_extents t) {
    print_state *&ptr =
        (print_state *&)os.pword(get_stream_manipulation_index());
    if (ptr) {
        ptr->num_elements_begin = t.begin;
    } else {
        ptr = new print_state(0, t.begin, t.end);
    }
    return os;
}

template<typename T>
size_t getw() {
    return 2;
}

template<>
size_t getw<long long>() {
    return 21;
}

template<>
size_t getw<int>() {
    return 12;
}

template<>
size_t getw<short>() {
    return 7;
}

template<>
size_t getw<char>() {
    return 4;
}

template<>
size_t getw<unsigned char>() {
    return 5;
}

template<>
size_t getw<detail::cfloat>() {
    return 15;
}

template<>
size_t getw<detail::cdouble>() {
    return 15;
}

template<>
size_t getw<float>() {
    return 7;
}

template<>
size_t getw<double>() {
    return 7;
}

template<typename T>
size_t getprecision() {
    return 4;
}

template<>
size_t getprecision<detail::cfloat>() {
    return 3;
}

template<>
size_t getprecision<detail::cdouble>() {
    return 3;
}

  // std::ostream &operator<<(std::ostream &os, af::cdouble vec) {
  //     // complex values seem to handle setw correctly
  //     std::complex<double> val(real(vec), imag(vec));
  //     os << val;
  //     return os;
  // }
  // 
  // std::ostream &operator<<(std::ostream &os, af::cfloat vec) {
  //     // complex values seem to handle setw correctly
  //     std::complex<float> val(real(vec), imag(vec));
  //     os << val;
  //     return os;
  // }

int integer_width(long long val) {
    int digits = 1;
    while (val > 9) {
        digits++;
        val /= 10;
    }
    return digits;
}
}  // namespace

#ifdef AF_CPU
namespace cpu {
#elif defined(AF_CUDA)
namespace cuda {
#elif defined(AF_OPENCL)
namespace opencl {
#endif

static std::ostream &operator<<(std::ostream &os, af::dtype type);

template<typename T>
std::ostream &operator<<(std::ostream &os, const detail::Array<T> &arr);

}  // namespace <backend>

namespace debugging {

template<typename T>
std::ostream &operator<<(std::ostream &os, std::vector<T> vec) {
    os << "[" << vec.size() << "]{";

    int elem = std::min(size_t(10), vec.size());
    for (int i = 0; i < elem; i++) {
        os << vec[i] << ((i < (elem - 1)) ? ", " : "");
    }
    os << "}";
    return os;
}

template<typename first>
void print(const char *F, first FF) {
    std::cerr << F << ": " << FF << "\n";
}

template<typename first, typename... ARGS>
void print(const char *F, first FF, ARGS... args) {
    std::cerr << F << ": " << FF << " ";
    print(args...);
}

template<typename ELEMENT, template<typename T> typename CONTAINER>
void head(const char *name, CONTAINER<ELEMENT> &var) {
    static_assert(1, "NOT IMPLEMENTED");
}

#define __FILENAME__ \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define SHOW1(val1) debugging::print(#val1, val1)
#define SHOW2(val1, val2) debugging::print(#val1, val1, #val2, val2)
#define SHOW3(val1, val2, val3) \
    debugging::print(#val1, val1, #val2, val2, #val3, val3)

#define SHOW4(val1, val2, val3, val4) \
    debugging::print(#val1, val1, #val2, val2, #val3, val3, #val4, val4)
#define SHOW5(val1, val2, val3, val4, val5)                              \
    debugging::print(#val1, val1, #val2, val2, #val3, val3, #val4, val4, \
                     #val5, val5)

#define GET_MACRO(_1, _2, _3, _4, _5, NAME, ...) NAME

#define SHOW(...)                                                 \
    do {                                                          \
        std::cerr << __FILENAME__ << "(" << __LINE__ << "): ";    \
        GET_MACRO(__VA_ARGS__, SHOW5, SHOW4, SHOW3, SHOW2, SHOW1) \
        (__VA_ARGS__);                                            \
    } while (0)

#define HEAD(arr)                                                              \
    do {                                                                       \
        using namespace debugging;                                             \
        std::cerr << __FILENAME__ << "(" << __LINE__ << "): " << #arr << ": "; \
        std::cerr << with_info << with_extents({8, 8, 1, 1}) << arr;           \
    } while (0)

}  // namespace debugging
