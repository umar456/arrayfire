/*******************************************************
 * Copyright (c) 2022, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once

#include <af/array.h>
#include <af/defines.h>
#include <af/dim4.hpp>
#include <af/event.h>

#ifdef __cplusplus
extern "C" {
#endif

enum af_copy_engine_type {
    AF_COPY_ENGINE_QUEUE,
    AF_COPY_ENGINE_LATEST,
    AF_COPY_ENGINE_BATCH_FIXED,
    AF_COPY_ENGINE_BATCH_VARIABLE,
};

enum af_copy_engine_properties : int {
    AF_COPY_ENGINE_PROPERTY_END,
    /// The maximum size of the queue or the batch array
    AF_COPY_ENGINE_PROPERTY_MAX_SIZE,

    /// The number of milliseconds to wait before the get function
    /// returns in case no new values are uploaded
    AF_COPY_ENGINE_PROPERTY_GET_TIMEOUT_MS,

    /// The number of milliseconds to wait before the upload function
    /// returns in case the queue is full
    AF_COPY_ENGINE_PROPERTY_UPLOAD_TIMEOUT_MS
};

enum af_copy_engine_result {
    /// The copy_engine function successully copied data
    AF_COPY_ENGINE_RESULT_SUCCESS = 0,

    /// The copy_engine function exited with an error and cannot continue
    /// processing.
    AF_COPY_ENGINE_RESULT_ERROR = 1 << 1,

    /// The copy_engine function finished processing
    AF_COPY_ENGINE_RESULT_DONE = 1 << 2,

    /// The copy_engine function skipped over the data element due to
    /// an error but is able to continue processing.
    AF_COPY_ENGINE_RESULT_SKIP = 1 << 3,
};

enum af_copy_engine_status {
    /// The copy_engine has been created but has not started doing work
    AF_COPY_ENGINE_STATUS_CREATED,

    /// The copy_engine has started loading data to the device
    AF_COPY_ENGINE_STATUS_STARTED,

    /// The copy_engine has finished loading data to the device but there
    /// is still some data available on the copy_engine queue
    AF_COPY_ENGINE_STATUS_STOPPED,

    /// The copy_engine has finished loading work and the work queue is empty
    AF_COPY_ENGINE_STATUS_DONE,
};

/// Handle to the copy_engine object. This object is used to copy data to the
/// device asynchronously while other operations are being performed by the
/// ArrayFire library.
typedef void *af_copy_engine;

/// A function pointer that will be called by the af_copy_engine object every
/// time a new array needs to be copied to the device.
///
/// \param[out] data         A host side pointer that will be uploaded to the
///                          device once the function returns.
/// \param[in/out] user_data A pointer to user assigned data passed during the
///                          creation of the af_copy_engine object
typedef af_copy_engine_result (*af_copy_engine_function)(void * /*data*/,
                                                         void * /*user_data*/);

/// Creates a new copy_engine object
AFAPI af_err af_create_copy_engine(af_copy_engine *copy_engine,
                                   const af_copy_engine_type copy_engine_type,
                                   const unsigned ndims, const dim_t *dims,
                                   af_dtype type,
                                   af_copy_engine_function function,
                                   af_copy_engine_properties *properties,
                                   void *user_data);

AFAPI af_err af_get_copy_engine_status(af_copy_engine_status *status,
                                       af_copy_engine copy_engine);

AFAPI af_err
af_get_copy_engine_property(int *val, const af_copy_engine_properties property,
                            const af_copy_engine copy_engine);

AFAPI af_err af_set_copy_engine_property(
    const af_copy_engine copy_engine, const af_copy_engine_properties property,
    const int val);

AFAPI af_err af_retain_copy_engine(af_copy_engine *out,
                                   af_copy_engine copy_engine);

AFAPI af_err af_release_copy_engine(af_copy_engine copy_engine);

AFAPI af_err af_start_copy_engine(af_copy_engine copy_engine);

AFAPI af_err af_stop_copy_engine(af_copy_engine copy_engine);

AFAPI af_err af_next_array_copy_engine(af_copy_engine copy_engine,
                                       af_array *arr);

/// Copies the host side memory at the \p data pointer to the device.
///
/// The upload function copies the data pointed at by the \p data pointer to the
/// device. This operation will be performed in a separate stream/queue so the
/// copy operation can be performed while other work is being computed by the
/// ArrayFire library.
///
/// This function should be called from a separate thread that is performing
/// work to load data on the host.
///
AFAPI af_err af_upload_copy_engine(af_copy_engine copy_engine, void *data,
                                   size_t size, af_copy_engine_result result,
                                   size_t num_events, af_event *wait_for,
                                   af_event upload_event);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace af {

class AFAPI copyEngine {
   private:
    af_copy_engine ce_;

   public:
    copyEngine(af_copy_engine loader);

    copyEngine(af_copy_engine_type ce_type, af::dim4 dims, af_dtype type,
               af_copy_engine_function function,
               af_copy_engine_properties *properties = NULL,
               void *user_data                       = NULL);
    ~copyEngine();

    void start();

    void stop();

    af_copy_engine get();

    af::array getNext();

    af::event upload(void *data, size_t size, af_copy_engine_result result);

    void uploadSync(void *data, size_t size, af_copy_engine_result result);
};
}  // namespace af
#endif
