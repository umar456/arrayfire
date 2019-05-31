/*******************************************************
 * Copyright (c) 2019, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/
#pragma once

#include <common/EventBase.hpp>
#include <cuda_runtime_api.h>

namespace cuda {

class CUDARuntimeEventPolicy {
   public:
    using EventType = cudaEvent_t;
    using QueueType = cudaStream_t;
    using ErrorType = cudaError_t;

    static cudaError_t createEvent(cudaEvent_t *e) noexcept {
        auto err = cudaEventCreate(e);
        // printf("create %p: error: %s\n", *e, cudaGetErrorName(err));
        return err;
    }

    static cudaError_t markEvent(cudaEvent_t *e,
                                 cudaStream_t &stream) noexcept {
        auto err = cudaEventRecord(*e, stream);
        // printf("mark   %p: error: %s\n", *e, cudaGetErrorName(err));
        return err;
    }

    static cudaError_t waitForEvent(cudaEvent_t *e,
                                    cudaStream_t &stream) noexcept {
        auto err = cudaStreamWaitEvent(stream, *e, 0);
        // printf("wait   %p: error: %s\n", *e, cudaGetErrorName(err));
        return err;
    }

    static cudaError_t syncForEvent(cudaEvent_t *e) noexcept {
        return cudaEventSynchronize(*e);
    }

    static cudaError_t destroyEvent(cudaEvent_t *e) noexcept {
        auto err = cudaEventDestroy(*e);
        // printf("destroy %p: error: %s\n", *e, cudaGetErrorName(err));
        return err;
    }
};

using Event = common::EventBase<CUDARuntimeEventPolicy>;

/// \brief Creates a new event and marks it in the stream
Event make_event(cudaStream_t stream);

}  // namespace cuda
