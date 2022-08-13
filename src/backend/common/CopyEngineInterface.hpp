/*******************************************************
 * Copyright (c) 2022, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#include <Policy.hpp>
#include <common/Event.hpp>
#include <common/Logger.hpp>

#include <af/dim4.hpp>
#include <af/copy_engine.h>

#include <nonstd/span.hpp>
#include <chrono>
#include <utility>

#pragma once
namespace common {

class CopyEngineInterface {
   public:
    CopyEngineInterface(
        std::function<af_copy_engine_result(void*, void*)> load_func,
        void* user_data, af::dim4 load_shape, af::dtype load_dtype,
        af_copy_engine_type l, unsigned nLoad,
        std::chrono::milliseconds get_timeout,
        std::chrono::milliseconds upload_timeout)
        : user_data_(user_data)
        , load_func_(load_func)
        , load_shape_(load_shape)
        , load_dtype_(load_dtype)
        , ce_type_(l)
        , nLoad_(nLoad)
        , status_(AF_COPY_ENGINE_STATUS_CREATED)
        , get_timeout_(get_timeout)
        , upload_timeout_(upload_timeout)
        , logger_(common::loggerFactory("loader")){};

    virtual operator bool()                   = 0;
    virtual std::pair<af_array, af_err> get() = 0;
    virtual ~CopyEngineInterface()            = default;

    virtual void start() = 0;

    virtual void stop() = 0;

    af_copy_engine_status& status() { return status_; }

    virtual af_err upload(void* data, size_t size,
                          nonstd::span<Event> wait_events,
                          common::Event& upload_event,
                          af_copy_engine_result result) = 0;

    std::chrono::milliseconds& getGetTimeout() { return get_timeout_; }

    std::chrono::milliseconds& getUploadTimeout() { return upload_timeout_; }

    unsigned& getMaxSize() { return nLoad_; }

   protected:
    spdlog::logger* getLogger() const noexcept { return logger_.get(); }

    // user defined load function for copying from
    // user_data to pinned memory
    void* user_data_;

    std::function<af_copy_engine_result(void* upload_buffer, void* user_data)>
        load_func_;

    // The shape of each individual input
    af::dim4 load_shape_;

    // The shape of the data on the device which is the same as the load_shape_
    // with the batch size
    af::dim4 staging_size_;

    // data type of data loaded
    af::dtype load_dtype_;

    // how to handle multiple loads
    // can be AF_COPY_ENGINE_{LATEST,QUEUE,BATCH}
    af_copy_engine_type ce_type_;

    unsigned nLoad_;

    // The status of the loader class
    af_copy_engine_status status_;

    /// The time before the get function returns with a timeout error
    std::chrono::milliseconds get_timeout_;

    /// The time before the upload function returns with a timeout error
    std::chrono::milliseconds upload_timeout_;

    // Logging
    std::shared_ptr<spdlog::logger> logger_;
};

}  // namespace common
