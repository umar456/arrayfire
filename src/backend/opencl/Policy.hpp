/*******************************************************
 * Copyright (c) 2022, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once

#include <cl2hpp.hpp>
#include <memory.hpp>
#include <platform.hpp>

#include <common/err_common.hpp>

namespace opencl {

class Policy {
   public:
    using EventType  = cl_event;
    using QueueType  = cl_command_queue;
    using ErrorType  = cl_int;
    using MemoryType = cl_mem;

    static const ErrorType SUCCESS_CODE = CL_SUCCESS;

    static ErrorType destroyQueue(QueueType queue) {
        return clReleaseCommandQueue(queue);
    }

    static void policy_assert(ErrorType err, const char *file = __FILE__,
                              int line = __LINE__) {
        if (err != CL_SUCCESS) {
            char err_msg[1024];
            snprintf(err_msg, sizeof(err_msg), "%s:%d OpenCL Error: %d", file,
                     line, err);
            boost::stacktrace::stacktrace st(1, 999);
            throw AfError(st.begin()->name().c_str(), file, line,
                          (const char *)err_msg, AF_ERR_INTERNAL, st);
        }
    }

    static ErrorType createQueue(QueueType *q) {
        cl_int success = CL_SUCCESS;
        *q = clCreateCommandQueue(getContext()(), getDevice()(), 0, &success);
        return success;
    }

    static ErrorType memcpyToDevice(QueueType q, MemoryType gpu_mem,
                                    void *pinned_mem, size_t write_bytes,
                                    size_t offset_bytes, EventType wait_event,
                                    EventType &write_event) {
        return clEnqueueWriteBuffer(q, gpu_mem, CL_FALSE, offset_bytes,
                                    write_bytes, pinned_mem, wait_event ? 1 : 0,
                                    wait_event ? &wait_event : nullptr,
                                    &write_event);
    }

    static MemoryType allocateMemory(size_t bytes, ErrorType *err = nullptr) {
        cl::Buffer *buf = memAlloc<char>(bytes).release();
        cl_mem out      = (*buf)();
        clRetainMemObject(out);
        delete buf;
        return out;
    }

    static ErrorType createEvent(EventType *e) { return CL_SUCCESS; }

    static ErrorType destroyEvent(EventType e) { return clReleaseEvent(e); }

    static ErrorType syncForEvents(int nEvents, EventType *events) {
        return clWaitForEvents(nEvents, events);
    }

    static ErrorType waitForEvents(QueueType queue, int nEvents,
                                   EventType *events) {
        return clEnqueueMarkerWithWaitList(queue, nEvents, events, nullptr);
    }

    static ErrorType markEvent(QueueType &queue, EventType &event) {
        return clEnqueueMarkerWithWaitList(queue, 0, nullptr, &event);
    }
};

}  // namespace opencl

#define POLICY_ASSERT(fn) \
    opencl::Policy::policy_assert((fn), __FILE__, __LINE__)
