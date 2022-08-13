
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
#include <queue>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <utility>

#include <iostream>

#include <Policy.hpp>
#include <backend.hpp>
#include <common/CopyEngineInterface.hpp>
#include <common/Logger.hpp>
#include <common/traits.hpp>
#include <handle.hpp>

namespace common {

template<typename LoaderBackendPolicy>
class QueueCopyEngine : public common::CopyEngineInterface {
    using QueueType  = typename LoaderBackendPolicy::QueueType;
    using ErrorType  = typename LoaderBackendPolicy::ErrorType;
    using MemoryType = typename LoaderBackendPolicy::MemoryType;
    using EventType  = typename LoaderBackendPolicy::EventType;

    // Maximum size of the output buffer
    int max_queue_size_;
    std::atomic<bool> done_;

    // Number of elements loaded in the current batch
    std::atomic<int> current_queue_count_;

    // upload queue
    QueueType upload_queue_;

    // ArrayFire queue
    QueueType arrayfire_queue_;

    std::array<std::pair<void *, common::Event>, 2> pinned_memory_pool_;

    // The event associated with the last memcpy
    common::Event last_event_;

    // Pointer where each data element will be stored
    std::queue<std::pair<MemoryType, common::Event>> current_staging_ptr_;

    // Mutex to protect the GPU array pointer
    std::mutex current_staging_ptr_mutex_;

    std::condition_variable cv;

    // The producer thread
    std::thread t_producer_;

    // Event used to synchronize the allocations on the other thread to the
    // arrayfire's queue
    Event upload_alloc_event_;

   public:
    QueueCopyEngine(
        QueueType arrayfire_queue,
        std::function<af_copy_engine_result(void *, void *)> load_func,
        void *user_data, af::dim4 load_shape, af::dtype load_dtype,
        unsigned nLoad, af_copy_engine_type queue_type,
        std::chrono::milliseconds get_timeout,
        std::chrono::milliseconds upload_timeout)
        : CopyEngineInterface(load_func, user_data, load_shape, load_dtype,
                              queue_type, nLoad, get_timeout, upload_timeout)
        , max_queue_size_(nLoad)
        , done_(false)
        , current_queue_count_(0)
        , arrayfire_queue_(arrayfire_queue)
        , last_event_() {
        // create upload queue, separate from arrayfire queue
        POLICY_ASSERT(LoaderBackendPolicy::createQueue(&upload_queue_));
        if (ce_type_ == AF_COPY_ENGINE_LATEST) { max_queue_size_ = 1; }
        size_t bytes_allocated =
            load_shape_.elements() * dtypeSize(load_dtype_);
        upload_alloc_event_.create();

        if (load_func) {
            for (std::pair<void *, common::Event> &pair : pinned_memory_pool_) {
                pair.first = detail::pinnedAlloc<void>(bytes_allocated);
                POLICY_ASSERT(pair.second.create());
            }
        }

        // prepare maximum batch-sized array for upload writes
        staging_size_ = load_shape;
    }

    void start() override {
        // initialize ingester thread that will run user function to copy data
        // to pinned memory then asynchronously upload to device
        if (load_func_) {
            std::thread tmp_producer(&QueueCopyEngine::ingest, this);
            std::swap(t_producer_, tmp_producer);
        }
    }

    void stop() override {
        done_ = true;
        cv.notify_one();
    }

    virtual ~QueueCopyEngine() {
        if (!done_) stop();
        if (t_producer_.joinable()) t_producer_.join();

        while (current_staging_ptr_.size()) {
            detail::memFree(current_staging_ptr_.front().first);
            current_staging_ptr_.pop();
        }

        if (load_func_) {
            detail::pinnedFree(pinned_memory_pool_[0].first);
            detail::pinnedFree(pinned_memory_pool_[1].first);
        }
        LoaderBackendPolicy::destroyQueue(upload_queue_);
    }

    void ingest() {
        using namespace std::chrono_literals;
        using common::Event;
        using std::array;
        using std::swap;

        detail::init();
        AF_TRACE("Starting ingest thread");
        status_ = AF_COPY_ENGINE_STATUS_STARTED;

        size_t load_bytes = load_shape_.elements() * dtypeSize(load_dtype_);

        while (!done_.load()) {
            swap(pinned_memory_pool_[0], pinned_memory_pool_[1]);

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
                cv.notify_one();
                break;
            }
        }
        if (current_queue_count_ == 0) {
            status_ = AF_COPY_ENGINE_STATUS_DONE;
        } else {
            status_ = AF_COPY_ENGINE_STATUS_STOPPED;
        }

        return;
    }

    af_err upload(void *data, size_t size, nonstd::span<Event> wait_events,
                  common::Event &upload_event,
                  af_copy_engine_result result) override {
        using namespace std::chrono_literals;
        using common::Event;
        using detail::memFree;
        using std::min;
        using std::mutex;
        using std::unique_lock;
        using std::chrono::duration_cast;
        using std::chrono::milliseconds;
        using std::chrono::steady_clock;

        status_ = AF_COPY_ENGINE_STATUS_STARTED;
        if (result & AF_COPY_ENGINE_RESULT_ERROR) {
            status_ = AF_COPY_ENGINE_STATUS_DONE;
            done_   = true;
            cv.notify_one();
            AF_TRACE("Upload result received an error result");
            return AF_SUCCESS;
        } else if (result & AF_COPY_ENGINE_RESULT_SKIP) {
            if (result & AF_COPY_ENGINE_RESULT_DONE) {
                status_ = AF_COPY_ENGINE_STATUS_STOPPED;
                done_   = true;
                cv.notify_one();
            }
            AF_TRACE("Upload result skipped");
            return AF_SUCCESS;
        } else if (result == AF_COPY_ENGINE_RESULT_SUCCESS ||
                   result & AF_COPY_ENGINE_RESULT_DONE) {
            auto before = steady_clock::now();
            std::vector<EventType> wait_native_events(std::begin(wait_events),
                                                      std::end(wait_events));

            unique_lock<mutex> lock(current_staging_ptr_mutex_);
            if (cv.wait_for(lock, 5s, [this] {
                    return done_ || ce_type_ == AF_COPY_ENGINE_LATEST ||
                           ((ce_type_ == AF_COPY_ENGINE_QUEUE) &&
                            (current_queue_count_ < max_queue_size_));
                })) {
                if (done_) { return AF_SUCCESS; }

                size_t bytes = load_shape_.elements() * dtypeSize(load_dtype_);
                size_t staged_bytes_allocated =
                    staging_size_.elements() * dtypeSize(load_dtype_);

                if (ce_type_ == AF_COPY_ENGINE_LATEST &&
                    (current_staging_ptr_.size() >
                     static_cast<size_t>(max_queue_size_ - 1))) {
                    memFree(current_staging_ptr_.front().first);
                    current_staging_ptr_.pop();
                    current_queue_count_--;
                }

                current_staging_ptr_.emplace(
                    LoaderBackendPolicy::allocateMemory(staged_bytes_allocated),
                    Event());
                current_queue_count_++;
                POLICY_ASSERT(current_staging_ptr_.back().second.create());
                cv.notify_one();

                AF_TRACE(
                    "Upload received success result current_batch_count: "
                    "{}/{}",
                    current_queue_count_.load(), max_queue_size_);

                upload_alloc_event_.mark(arrayfire_queue_);
                upload_alloc_event_.enqueueWait(upload_queue_);
                POLICY_ASSERT(LoaderBackendPolicy::memcpyToDevice(
                    upload_queue_, current_staging_ptr_.back().first, data,
                    min(size, bytes), 0,
                    {wait_native_events.data(), wait_native_events.size()},
                    current_staging_ptr_.back().second));
                if (upload_event) {
                    POLICY_ASSERT(upload_event.mark(upload_queue_));
                }

                if (result & AF_COPY_ENGINE_RESULT_DONE)
                    status_ = AF_COPY_ENGINE_STATUS_STOPPED;
            } else {
                AF_TRACE(
                    "Upload timed out after {}ms.",
                    duration_cast<milliseconds>(steady_clock::now() - before)
                        .count());
                return AF_ERR_COPY_ENGINE_TIMEOUT;
            }
        }

        return AF_SUCCESS;
    }

    std::pair<af_array, af_err> get() {
        using namespace std::chrono_literals;
        using af::dim4;
        using std::make_pair;
        using std::mutex;
        using std::unique_lock;
        using std::chrono::duration_cast;
        using std::chrono::milliseconds;
        using std::chrono::steady_clock;

        if (!t_producer_.joinable()) start();
        auto before = steady_clock::now();

        af_array out = nullptr;
        af_err err   = AF_SUCCESS;
        unique_lock<mutex> lock(current_staging_ptr_mutex_);
        if (cv.wait_for(lock, 5s, [this] {
                return done_ || !(current_queue_count_ == 0);
            })) {
            if (!done_) {
                before = steady_clock::now();
                if (current_queue_count_.load() > 0) {
                    AF_TRACE("Get success. current_queue_count_ {}/{}",
                             current_queue_count_, max_queue_size_);
                    POLICY_ASSERT(
                        current_staging_ptr_.front().second.enqueueWait(
                            arrayfire_queue_));

                    out = createHandleFromDeviceData(
                        load_shape_, load_dtype_,
                        current_staging_ptr_.front().first);

                    current_staging_ptr_.pop();
                    current_queue_count_--;
                    cv.notify_one();
                    if (status_ == AF_COPY_ENGINE_STATUS_STOPPED)
                        status_ = AF_COPY_ENGINE_STATUS_DONE;
                } else {
                    status_ = AF_COPY_ENGINE_STATUS_DONE;
                    AF_TRACE(
                        "Get function returning because the loader is "
                        "done");
                }
            }
        } else {
            AF_TRACE("get lock timed out after {}ms.",
                     duration_cast<milliseconds>(steady_clock::now() - before)
                         .count());
            err = AF_ERR_COPY_ENGINE_TIMEOUT;
        }
        return make_pair(out, err);
    }

    operator bool() { return !done_.load() || current_queue_count_ != 0; }
};

}  // namespace common
