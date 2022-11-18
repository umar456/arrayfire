/*******************************************************
 * Copyright (c) 2022, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#include <Array.hpp>
#include <common/dispatch.hpp>
#include <common/half.hpp>
#include <common/jit/ModdimNode.hpp>
#include <common/jit/Node.hpp>
#include <common/jit/NodeIterator.hpp>
#include <common/util.hpp>
#include <copy.hpp>
#include <device_manager.hpp>
#include <err_oneapi.hpp>
#include <threadsMgt.hpp>
#include <type_util.hpp>
#include <af/dim4.hpp>

#include <cuComplex.h>
#include "../cuda/jit.hpp"
#include <cuda.h>

#include <jit/BufferNode.hpp>

#include <cstdio>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using common::getFuncName;
using common::half;
using common::ModdimNode;
using common::Node;
using common::Node_ids;
using common::Node_map_t;
using common::Node_ptr;
using common::NodeIterator;

using oneapi::jit::BufferNode;

using std::string;
using std::stringstream;
using std::to_string;
using std::vector;

namespace cuda {
int getActiveDeviceId() { return 0; }

int getMultiProcessorCount(int i) { return 20; }

unsigned getMemoryBusWidth(const int) { return 0; }
size_t getL2CacheSize(int) { return 0; }
unsigned getMaxParallelThreads(int) { return 0; }

CUstream getActiveStream() { return 0; }
}  // namespace cuda

namespace oneapi {

string getKernelString(const string& funcName, const vector<Node*>& full_nodes,
                       const vector<Node_ids>& full_ids,
                       const vector<int>& output_ids, bool is_linear) {
    ONEAPI_NOT_SUPPORTED("");

    return "";
}

/*
cl::Kernel getKernel(const vector<Node *> &output_nodes,
                     const vector<int> &output_ids,
                     const vector<Node *> &full_nodes,
                     const vector<Node_ids> &full_ids, const bool is_linear) {
    ONEAPI_NOT_SUPPORTED("");
    return common::getKernel("", "", true).get();
}
*/

template<typename T>
void evalNodes(vector<Param<T>>& outputs, const vector<Node*>& output_nodes) {
  printf("EVAL\n");
    getQueue().submit([&](sycl::handler& h) {
        vector<sycl::accessor<T>> accessors;
        accessors.reserve(outputs.size());
        vector<cuda::Param<T>> params;
        params.reserve(outputs.size());
        transform(begin(outputs), end(outputs), back_inserter(accessors),
                  [&](Param<T> p) { return p.data->get_access(h); });
        transform(begin(outputs), end(outputs), back_inserter(params),
                  [&](Param<T> p) {
                      return cuda::Param<T>(nullptr, p.info.dims,
                                            p.info.strides);
                  });

        h.host_task([=](sycl::interop_handle hh) {
            switch (hh.get_backend()) {
                case sycl::backend::ext_oneapi_cuda: {
                    CUcontext ctx =
                        hh.get_native_context<sycl::backend::ext_oneapi_cuda>();
                    auto err = cuCtxSetCurrent(ctx);
                    printf("err: %d\n", err);
                    for (int i = 0; i < outputs.size(); i++) {
                        printf("i: %d\n", i);
                        CUdeviceptr ptr =
                            hh.get_native_mem<sycl::backend::ext_oneapi_cuda>(
                                accessors[i]);
                        ((cuda::Param<T>&)params[i]).ptr = (T*)ptr;
                    }

                    auto stream =
                        hh.get_native_queue<sycl::backend::ext_oneapi_cuda>();
                    cuda::evalNodes((vector<cuda::Param<T>>&)params,
                                    output_nodes, stream);
                } break;
            }
        });
    }).wait();

    printf("EVAL DONE\n");
}

template<>
void evalNodes<std::complex<float>>(vector<Param<std::complex<float>>>& outputs,
                                    const vector<Node*>& output_nodes) {
    getQueue().submit([&](sycl::handler& h) {
        h.host_task([=](sycl::interop_handle hh) {
            switch (hh.get_backend()) {
                case sycl::backend::ext_oneapi_cuda: {
                    vector<cuda::Param<cuFloatComplex>> op;

                    // transform(begin(outputs), end(outputs),
                    // back_inserter(op)):
                    auto stream =
                        hh.get_native_queue<sycl::backend::ext_oneapi_cuda>();
                    cuda::evalNodes(op, output_nodes, stream);
                } break;
            }
        });
    });
}

template<>
void evalNodes<std::complex<double>>(
    vector<Param<std::complex<double>>>& outputs,
    const vector<Node*>& output_nodes) {
    getQueue().submit([&](sycl::handler& h) {
        h.host_task([=](sycl::interop_handle hh) {
            switch (hh.get_backend()) {
                case sycl::backend::ext_oneapi_cuda: {
                    vector<cuda::Param<cuDoubleComplex>> op;

                    // transform(begin(outputs), end(outputs),
                    // back_inserter(op)):
                    auto stream =
                        hh.get_native_queue<sycl::backend::ext_oneapi_cuda>();
                    cuda::evalNodes(op, output_nodes, stream);
                } break;
            }
        });
    });
}

template<typename T>
void evalNodes(Param<T> out, Node* node) {
    vector<Param<T>> outputs{out};
    vector<Node*> nodes{node};
    evalNodes(outputs, nodes);
}

template void evalNodes<float>(Param<float> out, Node* node);
template void evalNodes<double>(Param<double> out, Node* node);
template void evalNodes<cfloat>(Param<cfloat> out, Node* node);
template void evalNodes<cdouble>(Param<cdouble> out, Node* node);
template void evalNodes<int>(Param<int> out, Node* node);
template void evalNodes<uint>(Param<uint> out, Node* node);
template void evalNodes<char>(Param<char> out, Node* node);
template void evalNodes<uchar>(Param<uchar> out, Node* node);
template void evalNodes<intl>(Param<intl> out, Node* node);
template void evalNodes<uintl>(Param<uintl> out, Node* node);
template void evalNodes<short>(Param<short> out, Node* node);
template void evalNodes<ushort>(Param<ushort> out, Node* node);
template void evalNodes<half>(Param<half> out, Node* node);

template void evalNodes<float>(vector<Param<float>>& out,
                               const vector<Node*>& node);
template void evalNodes<double>(vector<Param<double>>& out,
                                const vector<Node*>& node);
template void evalNodes<cfloat>(vector<Param<cfloat>>& out,
                                const vector<Node*>& node);
template void evalNodes<cdouble>(vector<Param<cdouble>>& out,
                                 const vector<Node*>& node);
template void evalNodes<int>(vector<Param<int>>& out,
                             const vector<Node*>& node);
template void evalNodes<uint>(vector<Param<uint>>& out,
                              const vector<Node*>& node);
template void evalNodes<char>(vector<Param<char>>& out,
                              const vector<Node*>& node);
template void evalNodes<uchar>(vector<Param<uchar>>& out,
                               const vector<Node*>& node);
template void evalNodes<intl>(vector<Param<intl>>& out,
                              const vector<Node*>& node);
template void evalNodes<uintl>(vector<Param<uintl>>& out,
                               const vector<Node*>& node);
template void evalNodes<short>(vector<Param<short>>& out,
                               const vector<Node*>& node);
template void evalNodes<ushort>(vector<Param<ushort>>& out,
                                const vector<Node*>& node);
template void evalNodes<half>(vector<Param<half>>& out,
                              const vector<Node*>& node);

}  // namespace oneapi
