/*******************************************************
 * Copyright (c) 2022, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once
#include <arrayfire.h>
#include <array>
#include <atomic>
#include <chrono>
#include <functional>
#include <iterator>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <utility>

#include <Policy.hpp>
#include <backend.hpp>
#include <common/CopyEngineInterface.hpp>
#include <common/Logger.hpp>
#include <common/traits.hpp>
#include <handle.hpp>

namespace common {

template<typename LoaderBackendPolicy>
class BatchCopyEngine : public common::CopyEngineInterface {
    using QueueType  = typename LoaderBackendPolicy::QueueType;
    using ErrorType  = typename LoaderBackendPolicy::ErrorType;
    using MemoryType = typename LoaderBackendPolicy::MemoryType;
    using EventType  = typename LoaderBackendPolicy::EventType;

    // Maximum size of the output buffer
    int max_batch_size_;
    std::atomic<bool> done_;

    // Number of elements loaded in the current batch
    std::atomic<int> current_batch_count_;

    // upload queue
    QueueType upload_queue_;

    // ArrayFire queue
    QueueType arrayfire_queue_;

    std::array<std::pair<void *, common::Event>, 2> pinned_memory_pool_;

    // The event associated with the last memcpy
    common::Event last_event_;

    // Pointer where each data element will be stored
    MemoryType current_staging_ptr_;

    // Mutex to protect the GPU array pointer
    std::mutex current_staging_ptr_mutex_;

    std::condition_variable cv;

    // The producer thread
    std::thread t_producer_;

   public:
    BatchCopyEngine(
        QueueType arrayfire_queue,
        std::function<af_copy_engine_result(void *, void *)> load_func,
        void *user_data, af::dim4 load_shape, af::dtype load_dtype,
        unsigned nLoad, af_copy_engine_type batch_type,
        std::chrono::milliseconds get_timeout,
        std::chrono::milliseconds upload_timeout)
        : CopyEngineInterface(load_func, user_data, load_shape, load_dtype,
                              batch_type, nLoad, get_timeout, upload_timeout)
        , max_batch_size_(nLoad)
        , done_(false)
        , current_batch_count_(0)
        , arrayfire_queue_(arrayfire_queue)
        , last_event_() {
        // create upload queue, separate from arrayfire queue
        POLICY_ASSERT(LoaderBackendPolicy::createQueue(&upload_queue_));

        POLICY_ASSERT(last_event_.create());
        size_t bytes_allocated =
            load_shape_.elements() * dtypeSize(load_dtype_);

        for (std::pair<void *, common::Event> &pair : pinned_memory_pool_) {
            pair.first = detail::pinnedAlloc<void>(bytes_allocated);
            POLICY_ASSERT(pair.second.create());
        }

        // prepare maximum batch-sized array for upload writes
        staging_size_                     = load_shape;
        staging_size_[load_shape.ndims()] = max_batch_size_;

        size_t staged_bytes_allocated =
            staging_size_.elements() * dtypeSize(load_dtype_);
        current_staging_ptr_ =
            LoaderBackendPolicy::allocateMemory(staged_bytes_allocated);
    }

    void start() override {
        // initialize ingester thread that will run user function to copy data
        // to pinned memory then asynchronously upload to device
        if (load_func_) {
            std::thread tmp_producer(&BatchCopyEngine::ingest, this);
            std::swap(t_producer_, tmp_producer);
        }
    }

    void stop() override {
        done_ = true;
        cv.notify_one();
    }

    virtual ~BatchCopyEngine() {
        if (!done_) stop();
        if (t_producer_.joinable()) t_producer_.join();
        detail::pinnedFree(pinned_memory_pool_[0].first);
        detail::pinnedFree(pinned_memory_pool_[1].first);
        LoaderBackendPolicy::destroyQueue(upload_queue_);
    }

    void ingest() {
        using namespace std::chrono_literals;
        detail::init();
        AF_TRACE("Starting ingest thread");
        status_ = AF_COPY_ENGINE_STATUS_STARTED;

        size_t load_bytes = load_shape_.elements() * dtypeSize(load_dtype_);

        while (!done_.load()) {
            using namespace std::chrono_literals;
            std::swap(pinned_memory_pool_[0], pinned_memory_pool_[1]);

            af_copy_engine_result func_result;
            pinned_memory_pool_[0].second.block();
            // write user function to now available pinned memory
            func_result = load_func_(pinned_memory_pool_[0].first, user_data_);
            af_err err;
            do {
                err = upload(pinned_memory_pool_[0].first, load_bytes, {},
                             pinned_memory_pool_[0].second, func_result);
            } while (err == AF_ERR_COPY_ENGINE_TIMEOUT);

            if (func_result & AF_COPY_ENGINE_RESULT_ERROR) {
                done_ = true;
                break;
            }
        }
        if (current_batch_count_ == 0) {
            status_ = AF_COPY_ENGINE_STATUS_DONE;
        } else {
            status_ = AF_COPY_ENGINE_STATUS_STOPPED;
        }

        return;
    }

    af_err upload(void *data, size_t size,
                  nonstd::span<common::Event> wait_events,
                  common::Event &upload_event,
                  af_copy_engine_result result) override {
        using namespace std::chrono_literals;
        using std::min;
        using std::mutex;
        using std::unique_lock;
        using std::chrono::duration_cast;
        using std::chrono::microseconds;
        using std::chrono::seconds;
        using std::chrono::steady_clock;
        using std::this_thread::sleep_for;
        using std::this_thread::yield;

        status_ = AF_COPY_ENGINE_STATUS_STARTED;

        af_err err = AF_SUCCESS;
        if (result & AF_COPY_ENGINE_RESULT_ERROR) {
            status_ = AF_COPY_ENGINE_STATUS_DONE;
            done_   = true;
            cv.notify_one();
            AF_TRACE("Upload result received an error result");
            err = AF_SUCCESS;
        } else if (result & AF_COPY_ENGINE_RESULT_SKIP) {
            if (result & AF_COPY_ENGINE_RESULT_DONE) {
                status_ = AF_COPY_ENGINE_STATUS_STOPPED;
                done_   = true;
                cv.notify_one();
            }
            AF_TRACE("Upload result skipped");
            err = AF_SUCCESS;
        } else if (result == AF_COPY_ENGINE_RESULT_SUCCESS ||
                   result & AF_COPY_ENGINE_RESULT_DONE) {
            auto before = steady_clock::now();
            AF_TRACE(
                "Upload received success result current_batch_count: {} max: "
                "{}",
                current_batch_count_.load(), max_batch_size_);

            std::vector<EventType> wait_native_events;
            wait_native_events.reserve((wait_events.size()));
            std::copy(std::begin(wait_events), std::end(wait_events),
                      std::back_inserter(wait_native_events));

            unique_lock<mutex> lock(current_staging_ptr_mutex_);
            if (cv.wait_for(lock, upload_timeout_, [this] {
                    return done_ || current_batch_count_ < max_batch_size_;
                })) {
                if (!done_) {
                    size_t bytes =
                        load_shape_.elements() * dtypeSize(load_dtype_);

                    POLICY_ASSERT(LoaderBackendPolicy::memcpyToDevice(
                        upload_queue_, current_staging_ptr_, data,
                        min(size, bytes), current_batch_count_.load() * bytes,
                        {wait_native_events.data(), wait_native_events.size()},
                        last_event_));
                    if (upload_event) {
                        POLICY_ASSERT(upload_event.mark(upload_queue_));
                    }

                    current_batch_count_++;
                    cv.notify_one();
                    if (result & AF_COPY_ENGINE_RESULT_DONE) {
                        status_ = AF_COPY_ENGINE_STATUS_STOPPED;
                    }
                }
            } else {
                AF_TRACE("Upload timed out after {}s.",
                         duration_cast<seconds>(steady_clock::now() - before)
                             .count());
                err = AF_ERR_COPY_ENGINE_TIMEOUT;
            }
        }

        return err;
    }

    std::pair<af_array, af_err> get() {
        using namespace std::chrono_literals;
        using af::dim4;
        using std::make_pair;
        using std::mutex;
        using std::unique_lock;
        using std::chrono::duration_cast;
        using std::chrono::seconds;
        using std::chrono::steady_clock;

        if (!t_producer_.joinable()) start();
        auto before  = steady_clock::now();
        af_array out = nullptr;
        af_err err   = AF_ERR_INTERNAL;
        before       = steady_clock::now();
        unique_lock<mutex> lock(current_staging_ptr_mutex_);
        if (cv.wait_for(lock, get_timeout_, [this] {
                return done_ ||
                       ((ce_type_ == AF_COPY_ENGINE_BATCH_VARIABLE &&
                         current_batch_count_ > 0) ||
                        (ce_type_ == AF_COPY_ENGINE_BATCH_FIXED &&
                         current_batch_count_ == max_batch_size_));
            })) {
            if (!done_) {
                AF_TRACE("Get success. current_batch_count_ {}",
                         current_batch_count_);
                POLICY_ASSERT(last_event_.enqueueWait(arrayfire_queue_));
                dim4 out_shape = load_shape_;

                switch (load_shape_.ndims()) {
                    case 1: out_shape[1] = current_batch_count_; break;
                    case 2: out_shape[2] = current_batch_count_; break;
                    case 3: out_shape[3] = current_batch_count_; break;
                }
                out = createHandleFromDeviceData(out_shape, load_dtype_,
                                                 current_staging_ptr_);

                size_t staged_bytes_allocated =
                    staging_size_.elements() * dtypeSize(load_dtype_);
                current_staging_ptr_ =
                    LoaderBackendPolicy::allocateMemory(staged_bytes_allocated);
                current_batch_count_ = 0;
                if (status_ == AF_COPY_ENGINE_STATUS_STOPPED) {
                    status_ = AF_COPY_ENGINE_STATUS_DONE;
                }
                err = AF_SUCCESS;
            } else {
                status_ = AF_COPY_ENGINE_STATUS_DONE;
                AF_TRACE("Get function returning because the loader is done");
                err = AF_SUCCESS;
            }
        } else {
            AF_TRACE(
                "get lock timed out after {}s.",
                duration_cast<seconds>(steady_clock::now() - before).count());
            err = AF_ERR_COPY_ENGINE_TIMEOUT;
        }
        cv.notify_one();
        return make_pair(out, err);
    }

    operator bool() { return !done_.load() || current_batch_count_ != 0; }
};

}  // namespace common
