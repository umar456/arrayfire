/*******************************************************
 * Copyright (c) 2022, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once

#include <nonstd/span.hpp>

#include <memory.hpp>
#include <platform.hpp>

#include <cu_check_macro.hpp>
#include <cuda.h>

namespace cuda {

class Policy {
   public:
    using EventType  = CUevent;
    using QueueType  = CUstream;
    using ErrorType  = CUresult;
    using MemoryType = void *;

    static ErrorType createQueue(QueueType *q) noexcept {
        return cuStreamCreate(q, CU_STREAM_NON_BLOCKING);
    }

    static ErrorType destroyQueue(QueueType queue) noexcept {
        return cuStreamDestroy(queue);
    }

    static void policy_assert(ErrorType err, const char *file, int line) {
        CUresult res = err;
        if (res == CUDA_SUCCESS) return;
        char cu_err_msg[1024];
        const char *cu_err_name;
        const char *cu_err_string;
        cuGetErrorName(res, &cu_err_name);
        cuGetErrorString(res, &cu_err_string);
        snprintf(cu_err_msg, sizeof(cu_err_msg),
                 "CUDA Driver Error %s(%d)(%s:%d): %s\n", cu_err_name,
                 (int)(res), file, line, cu_err_string);
        AF_ERROR(cu_err_msg, AF_ERR_INTERNAL);
    }
    static ErrorType memcpyToDevice(QueueType &q, MemoryType gpu_mem,
                                    MemoryType pinned_mem, size_t write_bytes,
                                    size_t offset_bytes,
                                    nonstd::span<EventType> wait_events,
                                    EventType &write_event) noexcept {
        ErrorType err = CUDA_SUCCESS;
        if (!wait_events.empty()) {
            for (EventType event : wait_events) {
                err = cuStreamWaitEvent(q, event, CU_EVENT_WAIT_DEFAULT);
            }
        }
        if (!err) {
            err = cuMemcpyHtoDAsync(
                reinterpret_cast<CUdeviceptr>(static_cast<char *>(gpu_mem) +
                                              offset_bytes),
                pinned_mem, write_bytes, q);
        }
        if (!err && write_event) { err = cuEventRecord(write_event, q); }
        return err;
    }

    static MemoryType allocateMemory(size_t bytes) {
        return memAlloc<char>(bytes).release();
    }

    static ErrorType createEvent(EventType *e) noexcept {
        // Creating events with the CU_EVENT_BLOCKING_SYNC flag
        // severly impacts the speed if/when creating many arrays
        return cuEventCreate(e, CU_EVENT_DISABLE_TIMING);
    }

    static ErrorType markEvent(QueueType &stream, EventType &event) noexcept {
        return cuEventRecord(event, stream);
    }

    static ErrorType waitForEvents(QueueType queue, int nEvents,
                                   EventType *events) noexcept {
        ErrorType err = CUDA_SUCCESS;
        for (int i = 0; i < nEvents; i++) {
            err = cuStreamWaitEvent(queue, events[i], 0);
            if (err) return err;
        }
        return err;
    }

    static ErrorType syncForEvents(int nEvents, EventType *events) noexcept {
        ErrorType err = CUDA_SUCCESS;
        for (int i = 0; i < nEvents; i++) {
            err = cuEventSynchronize(events[i]);
            if (err) return err;
        }
        return err;
    }

    static ErrorType destroyEvent(EventType e) noexcept {
        return cuEventDestroy(e);
    }
};

}  // namespace cuda

#define POLICY_ASSERT(fn) cuda::Policy::policy_assert((fn), __FILE__, __LINE__)
