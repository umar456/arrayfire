/*******************************************************
 * Copyright (c) 2022, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#include <af/copy_engine.h>

#include <af/array.h>
#include "error.hpp"

namespace af {

copyEngine::copyEngine(af_copy_engine ce) : ce_() {
    AF_THROW(af_retain_copy_engine(&ce_, ce));
}

copyEngine::copyEngine(af_copy_engine_type ce_type, af::dim4 dims,
                       af_dtype type, af_copy_engine_function function,
                       af_copy_engine_properties* properties, void* user_data) {
    AF_THROW(af_create_copy_engine(&ce_, ce_type, dims.ndims(),
                                   dims.get(), type, function, properties,
                                   user_data));
}

copyEngine::~copyEngine() { af_release_copy_engine(ce_); }

void copyEngine::start() { AF_THROW(af_start_copy_engine(ce_)); }

void copyEngine::stop() { AF_THROW(af_stop_copy_engine(ce_)); }

af_copy_engine copyEngine::get() { return ce_; }

af::array copyEngine::getNext() {
    af_array out;
    AF_THROW(af_next_array_copy_engine(ce_, &out));
    return af::array(out);
}

af::event copyEngine::upload(void* data, size_t size,
                             af_copy_engine_result result) {
    af_event e;
    AF_THROW(
        af_upload_copy_engine(ce_, data, size, result, 0, nullptr, &e));
    return af::event(e);
}

void copyEngine::uploadSync(void* data, size_t size,
                            af_copy_engine_result result) {
    af_event e;
    AF_THROW(
        af_upload_copy_engine(ce_, data, size, result, 0, nullptr, &e));
    AF_THROW(af_block_event(e));
    return;
}
}  // namespace af
