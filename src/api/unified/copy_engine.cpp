/*******************************************************
 * Copyright (c) 2015, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#include <symbol_manager.hpp>
#include <af/copy_engine.h>

af_err af_create_copy_engine(af_copy_engine *af_copy_engine,
                             const af_copy_engine_type ce_type,
                             const unsigned ndims, const dim_t *dims,
                             af_dtype type, af_copy_engine_function function,
                             af_copy_engine_properties *properties,
                             void *user_data) {
    CALL(af_create_copy_engine, af_copy_engine, ce_type, ndims, dims, type,
         function, properties, user_data);
}

af_err af_get_copy_engine_status(af_copy_engine_status *status,
                                 af_copy_engine loader) {
    CALL(af_get_copy_engine_status, status, loader);
}

af_err af_get_copy_engine_property(int *val,
                                   const af_copy_engine_properties property,
                                   const af_copy_engine loader) {
    CALL(af_get_copy_engine_property, val, property, loader);
}

af_err af_set_copy_engine_property(const af_copy_engine loader,
                                   const af_copy_engine_properties property,
                                   const int val) {
    CALL(af_set_copy_engine_property, loader, property, val);
}

af_err af_retain_copy_engine(af_copy_engine *out, af_copy_engine loader) {
    CALL(af_retain_copy_engine, out, loader);
}

af_err af_release_copy_engine(af_copy_engine loader) {
    CALL(af_release_copy_engine, loader);
}

af_err af_start_copy_engine(af_copy_engine loader) {
    CALL(af_start_copy_engine, loader);
}

af_err af_stop_copy_engine(af_copy_engine loader) {
    CALL(af_stop_copy_engine, loader);
}

af_err af_next_array_copy_engine(af_copy_engine loader, af_array *arr) {
    CALL(af_next_array_copy_engine, loader, arr);
}

af_err af_upload_copy_engine(af_copy_engine loader, void *data, size_t size,
                             af_copy_engine_result result, size_t num_events,
                             af_event *wait_for, af_event upload_event) {
    CALL(af_upload_copy_engine, loader, data, size, result, num_events,
         wait_for, upload_event);
}
