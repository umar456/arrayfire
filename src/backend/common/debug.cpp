
#include <common/debug.hpp>
#include <copy.hpp>
#include <math.hpp>

int get_stream_manipulation_index() {
    static int i = std::ios_base::xalloc();
    return i;
}

#ifdef AF_CPU
namespace cpu {
#elif defined(AF_CUDA)
namespace cuda {
#elif defined(AF_OPENCL)
namespace opencl {
#endif

static std::ostream &operator<<(std::ostream &os, af::dtype type) {
    std::string name;
    switch (type) {
        case f32: name = "f32"; break;
        case c32: name = "c32"; break;
        case f64: name = "f64"; break;
        case c64: name = "c64"; break;
        case b8: name = "b8"; break;
        case s32: name = "s32"; break;
        case u32: name = "u32"; break;
        case u8: name = "u8"; break;
        case s64: name = "s64"; break;
        case u64: name = "u64"; break;
        case s16: name = "s16"; break;
        case u16: name = "u16"; break;
        default: assert(false && "Invalid type");
    }
    return os << name;
}

template<typename T>
std::ostream &operator<<(std::ostream &os, const Array<T> &arr) {
    using namespace std;
    detail::Array<T> tarr = transpose(arr, false);

    std::vector<T> hdata(arr.elements());
    copyData(hdata.data(), tarr);

    // The number of elements to print from the beginning of the array
    af::dim4 begin_elements = arr.dims();

    // The number of elements to print from the end of the array
    af::dim4 end_elements   = {0, 0, 0, 0};

    // The precision of floating point operations
    int precision = getprecision<T>();

    // This is the output stream state which can be set using manipulators
    // such as with_info and extents which define how the Array<T> will
    // be printed
    // By default the entire array should be printed
    print_state *os_state;
    if (os_state = static_cast<print_state *>(
            os.pword(get_stream_manipulation_index()))) {
        if (os_state->print_info) {
            os << "(" << af::dtype(arr.getType()) << ")[" << arr.dims() << "]["
               << arr.strides() << "]\n";
        }
        if (os_state->precision != -1) { precision = os_state->precision; }
        begin_elements = os_state->num_elements_begin;
        end_elements   = os_state->num_elements_end;
    }

    os << std::fixed << setprecision(precision);

    T *orig_ptr = hdata.data();

    // if the begin_element is -1 then all elements should be printed
    for (int i = 0; i < begin_elements.ndims(); i++) {
        if (begin_elements[i] == -1) begin_elements[i] = arr.dims()[i];
    }

    af::dim4 pdims    = begin_elements + end_elements;
    af::dim4 arr_dims = arr.dims();

    T *ptr = nullptr;

    // This is the width of the indexing integers that will be included on the
    // axis of the printed array
    af::dim4 index_widths = {integer_width(arr.dims()[0]),
                             max((int)getw<T>(), integer_width(arr.dims()[1])),
                             integer_width(arr.dims()[2]),
                             integer_width(arr.dims()[3])};

    // Print the top axis of the array
    // TODO(umar): this should also be disabled by default and set by the
    // print_state struct
    os << setw(index_widths[0]) << " ";
    for (int i = 0, b = begin_elements[1], e = end_elements[1]; i < pdims[1];
         i++, b--) {
        if (b > 0) {
            os << setw(index_widths[1]) << i;
        } else {
            os << setw(index_widths[1]) << arr_dims[1] - e;
            e--;
        }
        if (b == 1) { os << std::right << std::setw(index_widths[1]) << "..."; }
    }
    os << "\n";

    for (int l = 0; l < pdims[3]; ++l) {
        auto lptr = orig_ptr + l * arr.dims()[3];
        for (int k = 0; k < pdims[2]; ++k) {
            auto kptr = lptr + k * arr.dims()[2];
            if (k > 0) os << "\n";

            for (int d1 = 0, b = begin_elements[1], e = end_elements[1];
                 d1 < pdims[0]; ++d1, b--) {
                T *d1ptr = kptr;

                // Set the offset for the dim 1 pointer
                if (b > 0) {
                    d1ptr += d1 * arr.dims()[0];

                    // print the index
                    os << setw(index_widths[0]) << d1;
                } else {
                    d1ptr = (kptr + arr.strides()[2]) - e * arr.dims()[1];
                    if (b != 0) os << setw(index_widths[0]) << arr_dims[0] - e;
                    e--;
                }

                if (b == 0) {
                    os << setw(index_widths[0]) << " ";
                    for (int i = 0; i < pdims[1] + 1; i++) {
                        if (i == begin_elements[0])
                          os << std::right << std::setw(index_widths[1]) << " ⋱";
                        else
                          os << std::right << std::setw(index_widths[1]) << "...";
                    }
                    os << "\n" << arr_dims[0] - e - 1;
                }

                for (int d0 = 0, b = begin_elements[1], e = end_elements[1];
                     d0 < pdims[1]; d0++, b--) {
                    if (b > 0) {
                        os << setw(index_widths[1]) << +d1ptr[d0];
                    } else {
                        os << setw(index_widths[1]) << +d1ptr[arr.dims()[1] - e];
                        e--;
                    }
                    if (b == 1) {
                        os << std::right << std::setw(index_widths[1]) << "...";
                    }
                }
                os << "\n";
            }
            os << "\n";
        }
        os << "\n";
    }
    return os;
}
#define INSTANTIATE(TYPE) \
  template std::ostream &operator<<<TYPE>(std::ostream &os, const Array<TYPE> &arr)

  INSTANTIATE(float);
  INSTANTIATE(double);
  INSTANTIATE(cfloat);
  INSTANTIATE(cdouble);
  INSTANTIATE(unsigned int);
  INSTANTIATE(int);
  INSTANTIATE(short);
  INSTANTIATE(unsigned short);
  INSTANTIATE(char);
  INSTANTIATE(unsigned char);
  INSTANTIATE(long long);
  INSTANTIATE(unsigned long long);

}// namespace <backend>
