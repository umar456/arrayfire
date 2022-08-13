/*******************************************************
 * Copyright (c) 2022, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#include <Policy.hpp>
#include <backend.hpp>
#include <common/CopyEngineInterface.hpp>
#include <common/BatchCopyEngine.hpp>
#include <common/QueueCopyEngine.hpp>
#include <events.hpp>

#include <af/defines.h>
#include <af/dim4.hpp>
#include <af/event.h>
#include <af/copy_engine.h>

#include <map>
#include <memory>

using common::BatchCopyEngine;
using common::CopyEngineInterface;
using common::QueueCopyEngine;

using std::make_shared;
using std::map;
using std::shared_ptr;
using std::chrono::milliseconds;

map<af_copy_engine_properties, int> readLoaderProperties(
    af_copy_engine_properties *properties) {
    map<af_copy_engine_properties, int> out;
    out[AF_COPY_ENGINE_PROPERTY_MAX_SIZE]          = 2;
    out[AF_COPY_ENGINE_PROPERTY_GET_TIMEOUT_MS]    = 5000;
    out[AF_COPY_ENGINE_PROPERTY_UPLOAD_TIMEOUT_MS] = 5000;

    while (properties && *properties != AF_COPY_ENGINE_PROPERTY_END) {
        switch (*properties) {
            case AF_COPY_ENGINE_PROPERTY_MAX_SIZE:
                out[AF_COPY_ENGINE_PROPERTY_MAX_SIZE] = (int)properties[1];
                break;
            case AF_COPY_ENGINE_PROPERTY_GET_TIMEOUT_MS:
                out[AF_COPY_ENGINE_PROPERTY_GET_TIMEOUT_MS] =
                    (int)properties[1];
                break;
            case AF_COPY_ENGINE_PROPERTY_UPLOAD_TIMEOUT_MS:
                out[AF_COPY_ENGINE_PROPERTY_UPLOAD_TIMEOUT_MS] =
                    (int)properties[1];
                break;
            default: AF_ERROR("Invalid property value", AF_ERR_ARG);
        }
        properties += 2;
    }

    return out;
}

af_err af_create_copy_engine(af_copy_engine *loader,
                             const af_copy_engine_type ce_type,
                             const unsigned ndims, const dim_t *dims,
                             af_dtype type, af_copy_engine_function function,
                             af_copy_engine_properties *properties,
                             void *user_data) {
    try {
        af::dim4 dim(ndims, dims);
        auto queue = detail::getQueueHandle(detail::getActiveDeviceId());
        map<af_copy_engine_properties, int> properties_map =
            readLoaderProperties(properties);

        milliseconds get_timeout(
            properties_map[AF_COPY_ENGINE_PROPERTY_GET_TIMEOUT_MS]);
        milliseconds upload_timeout(
            properties_map[AF_COPY_ENGINE_PROPERTY_UPLOAD_TIMEOUT_MS]);

        if (ce_type == AF_COPY_ENGINE_BATCH_VARIABLE ||
            ce_type == AF_COPY_ENGINE_BATCH_FIXED) {
            auto *l = new shared_ptr<BatchCopyEngine<detail::Policy>>;
            *l      = make_shared<BatchCopyEngine<detail::Policy>>(
                queue, function, user_data, dim, type,
                properties_map[AF_COPY_ENGINE_PROPERTY_MAX_SIZE], ce_type,
                get_timeout, upload_timeout);

            *loader = static_cast<af_copy_engine>(l);
        } else if (ce_type == AF_COPY_ENGINE_QUEUE ||
                   ce_type == AF_COPY_ENGINE_LATEST) {
            auto *l = new shared_ptr<QueueCopyEngine<detail::Policy>>;
            *l      = make_shared<QueueCopyEngine<detail::Policy>>(
                queue, function, user_data, dim, type,
                properties_map[AF_COPY_ENGINE_PROPERTY_MAX_SIZE], ce_type,
                get_timeout, upload_timeout);
            *loader = static_cast<af_copy_engine>(l);
        }
    }
    CATCHALL
    return AF_SUCCESS;
}

af_err af_get_copy_engine_property(int *val,
                                   const af_copy_engine_properties property,
                                   const af_copy_engine loader) {
    auto &l = *static_cast<shared_ptr<CopyEngineInterface> *>(loader);
    switch (property) {
        case AF_COPY_ENGINE_PROPERTY_MAX_SIZE: *val = l->getMaxSize(); break;
        case AF_COPY_ENGINE_PROPERTY_GET_TIMEOUT_MS:
            *val = l->getGetTimeout().count();
            break;
        case AF_COPY_ENGINE_PROPERTY_UPLOAD_TIMEOUT_MS:
            *val = l->getUploadTimeout().count();
            break;
        default: AF_ERROR("Invalid property value", AF_ERR_ARG);
    }
    return AF_SUCCESS;
}

af_err af_set_copy_engine_property(const af_copy_engine loader,
                                   const af_copy_engine_properties property,
                                   const int val) {
    auto &l = *static_cast<shared_ptr<CopyEngineInterface> *>(loader);
    switch (property) {
        case AF_COPY_ENGINE_PROPERTY_MAX_SIZE: l->getMaxSize() = val; break;
        case AF_COPY_ENGINE_PROPERTY_GET_TIMEOUT_MS:
            l->getGetTimeout() = milliseconds(val);
            break;
        case AF_COPY_ENGINE_PROPERTY_UPLOAD_TIMEOUT_MS:
            l->getUploadTimeout() = milliseconds(val);
            break;
        default: AF_ERROR("Invalid property value", AF_ERR_ARG);
    }
    return AF_SUCCESS;
}

af_err af_get_copy_engine_status(af_copy_engine_status *status,
                                 af_copy_engine loader) {
    try {
        auto &l = *static_cast<shared_ptr<CopyEngineInterface> *>(loader);

        *status = l->status();
    }
    CATCHALL
    return AF_SUCCESS;
}

af_err af_retain_copy_engine(af_copy_engine *out, af_copy_engine loader) {
    try {
        auto *l  = static_cast<shared_ptr<CopyEngineInterface> *>(loader);
        auto *l2 = new shared_ptr<CopyEngineInterface>(*l);
        *out     = static_cast<af_copy_engine>(l2);
    }
    CATCHALL
    return AF_SUCCESS;
}

af_err af_release_copy_engine(af_copy_engine loader) {
    try {
        auto *l = static_cast<shared_ptr<CopyEngineInterface> *>(loader);
        delete l;
    }
    CATCHALL
    return AF_SUCCESS;
}

af_err af_start_copy_engine(af_copy_engine loader) {
    try {
        auto &l = *static_cast<shared_ptr<CopyEngineInterface> *>(loader);
        l->start();
    }
    CATCHALL
    return AF_SUCCESS;
}

af_err af_stop_copy_engine(af_copy_engine loader) {
    try {
        auto &l = *static_cast<shared_ptr<CopyEngineInterface> *>(loader);
        l->stop();
    }
    CATCHALL
    return AF_SUCCESS;
}

af_err af_next_array_copy_engine(af_copy_engine loader, af_array *arr) {
    af_err err = AF_SUCCESS;
    try {
        auto &l = *static_cast<shared_ptr<CopyEngineInterface> *>(loader);
        std::tie(*arr, err) = l->get();
    }
    CATCHALL

    return err;
}

af_err af_upload_copy_engine(af_copy_engine loader, void *data, size_t size,
                             af_copy_engine_result result, size_t nevents,
                             af_event *wait_for, af_event upload_event) {
    af_err err = AF_SUCCESS;
    try {
        AF_CHECK(af_init());
        auto &l = *static_cast<shared_ptr<CopyEngineInterface> *>(loader);

        if (!data) {
            common::Event e;
            err = l->upload(nullptr, 0, {}, e, result);
            return err;
        }

        af_event event;
        if (upload_event) {
            event = upload_event;
        } else {
            /// Upload event is nullptr so create a new event.
            AF_CHECK(af_create_event(&event));
        }

        common::Event &ev = getEvent(event);
        ARG_ASSERT(5, nevents == 0 || (nevents > 0 && wait_for != nullptr));

        if (nevents > 0) {
            err = l->upload(
                data, size,
                {*reinterpret_cast<common::Event **>(wait_for), nevents}, ev,
                result);
        } else {
            err = l->upload(data, size, {}, ev, result);
        }

        if (!upload_event) {
            AF_CHECK(af_block_event(event));
            AF_CHECK(af_delete_event(event));
        }
    }
    CATCHALL
    return err;
}
