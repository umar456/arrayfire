
#include <Array.hpp>
#include <all_reduce.hpp>
#include <common/half.hpp>
#include <platform.hpp>

#include <nccl.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <set>
#include <vector>

#define NCCLCHECK(cmd)                                                    \
    do {                                                                  \
        ncclResult_t r = cmd;                                             \
        if (r != ncclSuccess) {                                           \
            printf("Failed, NCCL error %s:%d '%s'\n", __FILE__, __LINE__, \
                   ncclGetErrorString(r));                                \
            exit(EXIT_FAILURE);                                           \
        }                                                                 \
    } while (0)

namespace cuda {
std::vector<ncclComm_t> comms(16);

template<typename T>
ncclDataType_t ncclType();

template<>
ncclDataType_t ncclType<float>() {
    return ncclFloat32;
}

template<>
ncclDataType_t ncclType<double>() {
    return ncclFloat64;
}

template<>
ncclDataType_t ncclType<common::half>() {
    return ncclFloat16;
}

template<>
ncclDataType_t ncclType<int>() {
    return ncclInt32;
}

template<>
ncclDataType_t ncclType<unsigned>() {
    return ncclUint32;
}

template<>
ncclDataType_t ncclType<long>() {
    return ncclInt64;
}

template<>
ncclDataType_t ncclType<unsigned long>() {
    return ncclUint64;
}

template<>
ncclDataType_t ncclType<unsigned char>() {
    return ncclInt8;
}

template<af_op_t OP>
ncclRedOp_t ncclOp();

template<>
ncclRedOp_t ncclOp<af_add_t>() {
    return ncclSum;
}

template<af_op_t op, typename Ti>
void all_reduce(std::vector<Array<Ti> *> in) {
    std::set<int> devices;
    // for (Array<Ti> *arr : in) { devices.emplace(arr->getDevId()); }

    // if (devices.size() == 1) {
    // for (int i = 0; i < devices.size(); i++) {}
    //}

    static auto init = [&]() {
        NCCLCHECK(ncclCommInitAll(comms.data(), getDeviceCount(), nullptr));
        return true;
    }();

    std::vector<Ti *> ptrs(in.size());
    transform(begin(in), end(in), begin(ptrs),
              [](Array<Ti> *inarr) { return inarr->get(); });

    NCCLCHECK(ncclGroupStart());
    for (int i = 0; i < ptrs.size(); i++) {
        int devid = in[i]->getDevId();
        NCCLCHECK(ncclAllReduce(ptrs[i], ptrs[i], in[i]->elements(),
                                ncclType<Ti>(), ncclOp<op>(), comms[devid],
                                getStream(devid)));
    }
    NCCLCHECK(ncclGroupEnd());
}

template void all_reduce<af_add_t, float>(std::vector<Array<float> *> in);
}  // namespace cuda
