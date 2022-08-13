/*******************************************************
 * Copyright (c) 2022, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#include <gtest/gtest.h>
#include <testHelpers.hpp>
#include <af/arith.h>
#include <af/array.h>
#include <af/blas.h>
#include <af/copy_engine.h>
#include <af/data.h>
#include <af/event.h>

#include <atomic>
#include <chrono>
#include <sstream>
#include <thread>

using af::dim4;
using std::atomic;
using std::get;
using std::ostream;
using std::pair;
using std::replace;
using std::string;
using std::stringstream;
using std::thread;
using std::tuple;
using std::vector;
using std::chrono::milliseconds;
using std::this_thread::sleep_for;

ostream& operator<<(ostream& os, af_copy_engine_status status) {
    switch (status) {
        case AF_COPY_ENGINE_STATUS_CREATED:
            os << "AF_COPY_ENGINE_STATUS_CREATED";
            break;
        case AF_COPY_ENGINE_STATUS_STARTED:
            os << "AF_COPY_ENGINE_STATUS_STARTED";
            break;
        case AF_COPY_ENGINE_STATUS_STOPPED:
            os << "AF_COPY_ENGINE_STATUS_STOPPED";
            break;
        case AF_COPY_ENGINE_STATUS_DONE:
            os << "AF_COPY_ENGINE_STATUS_DONE";
            break;
    }
    return os;
}

ostream& operator<<(ostream& os, af_copy_engine_type type) {
    switch (type) {
        case AF_COPY_ENGINE_BATCH_VARIABLE: os << "BATCH_VARIABLE"; break;
        case AF_COPY_ENGINE_BATCH_FIXED: os << "BATCH_FIXED"; break;
        case AF_COPY_ENGINE_QUEUE: os << "QUEUE"; break;
        case AF_COPY_ENGINE_LATEST: os << "LATEST"; break;
    }
    return os;
}

struct ce_params {
    af_copy_engine_type type;
    dim4 shape;
    int max_batch_size;

    ce_params(tuple<af_copy_engine_type, dim4, int> in)
        : type(get<0>(in)), shape(get<1>(in)), max_batch_size(get<2>(in)) {}
};

typedef struct {
    int val;
    size_t elements;
    af_copy_engine_type ce_type;
} my_user_data;

af_copy_engine_result load_function(void* mem, void* user) {
    auto* user_data = (my_user_data*)user;
    // if (user_data->ce_type == AF_COPY_ENGINE_LATEST)
    // sleep_for(milliseconds(3));
    (user_data->val)++;

    float* mem_float = (float*)mem;
    for (size_t i = 0; i < user_data->elements; i++) {
        mem_float[i] = user_data->val;
    }

    if (user_data->val < 100000) { return AF_COPY_ENGINE_RESULT_SUCCESS; }
    return af_copy_engine_result(AF_COPY_ENGINE_RESULT_SUCCESS |
                                 AF_COPY_ENGINE_RESULT_DONE);
}

class CopyEngineInternal
    : public ::testing::TestWithParam<tuple<af_copy_engine_type, dim4, int> > {
    void SetUp() override {
        ce_params params(GetParam());
        ce = 0;

        data =
            new my_user_data{0, (size_t)params.shape.elements(), params.type};

        af_copy_engine_properties properties[] = {
            AF_COPY_ENGINE_PROPERTY_MAX_SIZE,
            static_cast<af_copy_engine_properties>(params.max_batch_size),
            AF_COPY_ENGINE_PROPERTY_END};

        dim_t* size = params.shape.get();
        ASSERT_SUCCESS(af_create_copy_engine(&ce, params.type,
                                             params.shape.ndims(), size, f32,
                                             load_function, properties, data));
    }

    void TearDown() override {
        ASSERT_SUCCESS(af_release_copy_engine(ce));
        delete data;
    }

   public:
    af_copy_engine ce;
    my_user_data* data;
};

string tupleToTestName(
    const ::testing::TestParamInfo<CopyEngineInternal::ParamType> info) {
    stringstream ss;
    ce_params params{info.param};
    ss << params.type << "_shape_" << params.shape << "_max_batch_size_"
       << params.max_batch_size;

    auto out = ss.str();
    replace(begin(out), end(out), ' ', '_');
    return out;
}

INSTANTIATE_TEST_CASE_P(
    CopyEngineFunction, CopyEngineInternal,
    ::testing::Combine(
        ::testing::Values(AF_COPY_ENGINE_LATEST, AF_COPY_ENGINE_QUEUE,
                          AF_COPY_ENGINE_BATCH_VARIABLE,
                          AF_COPY_ENGINE_BATCH_FIXED),
        ::testing::Values(dim4(100), dim4(100, 100), dim4(10, 10, 10)),
        ::testing::Values(1, 2, 4, 8, 16, 256)),
    tupleToTestName);

TEST_P(CopyEngineInternal, Loop) {
    ce_params params{GetParam()};
    ASSERT_SUCCESS(af_start_copy_engine(ce));

    af_array a       = 0;
    int slice_values = 0;
    float* gold_data;
    ASSERT_SUCCESS(af_alloc_pinned(
        (void**)&gold_data,
        params.shape.elements() * params.max_batch_size * sizeof(float)));
    for (int i = 0; i < 100; i++) {
        if (params.type == AF_COPY_ENGINE_LATEST) {
            ASSERT_SUCCESS(af_next_array_copy_engine(ce, &a));
            slice_values = data->val;
        } else {
            ASSERT_SUCCESS(af_next_array_copy_engine(ce, &a));
        }

        unsigned int ndims = 0;
        if (!a) { break; }
        ASSERT_SUCCESS(af_get_numdims(&ndims, a));

        dim_t dims[4];
        ASSERT_SUCCESS(af_get_dims(&dims[0], &dims[1], &dims[2], &dims[3], a));

        switch (params.type) {
            case AF_COPY_ENGINE_QUEUE:
            case AF_COPY_ENGINE_LATEST:
                ASSERT_EQ(params.shape, dim4(4, dims));
                break;
            case AF_COPY_ENGINE_BATCH_FIXED:
                ASSERT_EQ(params.max_batch_size, dims[params.shape.ndims()]);
                break;
            case AF_COPY_ENGINE_BATCH_VARIABLE:
                ASSERT_LE(dims[params.shape.ndims()], params.max_batch_size);
                break;
        }

        af_array out = 0;
        ASSERT_SUCCESS(af_add(&out, a, a, false));
        for (int i = 0; i < 5; i++) {
            af_array oout = 0;
            ASSERT_SUCCESS(af_add(&oout, out, a, false));
            ASSERT_SUCCESS(af_release_array(out));
            out = oout;
        }
        ASSERT_SUCCESS(af_eval(out));
        ASSERT_SUCCESS(af_release_array(out));

        size_t elements = 0;
        if (params.type == AF_COPY_ENGINE_BATCH_VARIABLE) {
            elements = dims[0] * dims[1] * dims[2] * dims[3];
        } else {
            elements = dims[params.shape.ndims()] * params.shape.elements();
        }

        elements       = params.shape.elements();
        int num_slices = dims[params.shape.ndims()];

        for (int ii = 0; ii < num_slices; ii++) {
            if (params.type != AF_COPY_ENGINE_LATEST) { slice_values++; }
            for (int jj = 0; jj < elements; jj++) {
                gold_data[elements * ii + jj] = slice_values;
            }
        }

        af_array gold = 0;
        ASSERT_SUCCESS(af_create_array(&gold, gold_data, ndims, dims, f32));

        if (params.type != AF_COPY_ENGINE_LATEST) {
            ASSERT_ARRAYS_EQ(gold, a) << af_print_array(a);
        }
        ASSERT_SUCCESS(af_release_array(gold));
        ASSERT_SUCCESS(af_release_array(a));
        a = 0;
    }

    ASSERT_SUCCESS(af_stop_copy_engine(ce));
    ASSERT_SUCCESS(af_free_pinned(gold_data));
}

TEST_P(CopyEngineInternal, LoopCpp) {
    ce_params params{GetParam()};
    af::copyEngine l(ce);
    l.start();
    // ASSERT_SUCCESS(af_start_copy_engine(ce));

    int slice_values = 0;
    float* gold_data;
    ASSERT_SUCCESS(af_alloc_pinned(
        (void**)&gold_data,
        params.shape.elements() * params.max_batch_size * sizeof(float)));
    for (int i = 0; i < 100; i++) {
        af::array a;
        try {
            if (params.type == AF_COPY_ENGINE_LATEST) {
                a            = l.getNext();
                slice_values = data->val;
            } else {
                a = l.getNext();
            }
        } catch (af::exception& ex) {
            if (ex.err() == AF_ERR_COPY_ENGINE_TIMEOUT) break;
        }

        switch (params.type) {
            case AF_COPY_ENGINE_QUEUE:
            case AF_COPY_ENGINE_LATEST:
                ASSERT_EQ(params.shape, a.dims());
                break;
            case AF_COPY_ENGINE_BATCH_FIXED:
                ASSERT_EQ(params.max_batch_size,
                          a.dims()[params.shape.ndims()]);
                break;
            case AF_COPY_ENGINE_BATCH_VARIABLE:
                ASSERT_LE(a.dims()[params.shape.ndims()],
                          params.max_batch_size);
                break;
        }

        af::array out = a + a;
        for (int i = 0; i < 5; i++) { out += a; }
        out.eval();

        size_t elements = 0;
        if (params.type == AF_COPY_ENGINE_BATCH_VARIABLE) {
            elements = a.elements();
        } else {
            elements = a.dims()[params.shape.ndims()] * params.shape.elements();
        }

        elements       = params.shape.elements();
        int num_slices = a.dims(params.shape.ndims());

        for (int ii = 0; ii < num_slices; ii++) {
            if (params.type != AF_COPY_ENGINE_LATEST) { slice_values++; }
            for (int jj = 0; jj < elements; jj++) {
                gold_data[elements * ii + jj] = slice_values;
            }
        }

        af::array gold(a.dims(), gold_data);

        if (params.type != AF_COPY_ENGINE_LATEST) {
            ASSERT_ARRAYS_EQ(gold, a) << af_print_array(a.get());
        }
    }

    l.stop();
    ASSERT_SUCCESS(af_free_pinned(gold_data));
}

TEST(CopyEngine, State) {
    af_array arr                           = 0;
    af_copy_engine ce                      = 0;
    af_copy_engine_properties properties[] = {
        AF_COPY_ENGINE_PROPERTY_MAX_SIZE,
        static_cast<af_copy_engine_properties>(10),
        AF_COPY_ENGINE_PROPERTY_GET_TIMEOUT_MS,
        static_cast<af_copy_engine_properties>(1000),
        AF_COPY_ENGINE_PROPERTY_UPLOAD_TIMEOUT_MS,
        static_cast<af_copy_engine_properties>(2000),
        AF_COPY_ENGINE_PROPERTY_END};

    my_user_data data{0, 10};
    dim_t size[] = {10};
    ASSERT_SUCCESS(af_create_copy_engine(&ce, AF_COPY_ENGINE_BATCH_VARIABLE, 1,
                                         size, f32, load_function, properties,
                                         &data));

    af_copy_engine_status state;
    ASSERT_SUCCESS(af_get_copy_engine_status(&state, ce));
    ASSERT_EQ(AF_COPY_ENGINE_STATUS_CREATED, state);

    ASSERT_SUCCESS(af_start_copy_engine(ce));
    sleep_for(milliseconds(200));
    ASSERT_SUCCESS(af_get_copy_engine_status(&state, ce));
    ASSERT_EQ(AF_COPY_ENGINE_STATUS_STARTED, state);

    af_array a = 0;
    ASSERT_SUCCESS(af_next_array_copy_engine(ce, &a));
    sleep_for(milliseconds(1));
    ASSERT_SUCCESS(af_get_copy_engine_status(&state, ce));
    ASSERT_EQ(AF_COPY_ENGINE_STATUS_STARTED, state);

    ASSERT_SUCCESS(af_release_array(a));
    ASSERT_SUCCESS(af_get_copy_engine_status(&state, ce));
    ASSERT_EQ(AF_COPY_ENGINE_STATUS_STARTED, state);

    ASSERT_SUCCESS(af_stop_copy_engine(ce));
    sleep_for(milliseconds(100));
    ASSERT_SUCCESS(af_get_copy_engine_status(&state, ce));
    ASSERT_EQ(AF_COPY_ENGINE_STATUS_STOPPED, state);

    ASSERT_SUCCESS(af_next_array_copy_engine(ce, &a));
    ASSERT_SUCCESS(af_release_array(a));
    ASSERT_SUCCESS(af_get_copy_engine_status(&state, ce));
    ASSERT_EQ(AF_COPY_ENGINE_STATUS_DONE, state);
    ASSERT_SUCCESS(af_release_copy_engine(ce));
}

TEST(CopyEngine, GetWithoutStartDoesntFreeze) {
    af_array arr                           = 0;
    af_copy_engine ce                      = 0;
    af_copy_engine_properties properties[] = {
        AF_COPY_ENGINE_PROPERTY_MAX_SIZE,
        static_cast<af_copy_engine_properties>(10),
        AF_COPY_ENGINE_PROPERTY_END};

    my_user_data data{0, 10};
    dim_t size[] = {10};
    ASSERT_SUCCESS(af_create_copy_engine(&ce, AF_COPY_ENGINE_BATCH_VARIABLE, 1,
                                         size, f32, load_function, properties,
                                         &data));
    ASSERT_SUCCESS(af_next_array_copy_engine(ce, &arr));
    ASSERT_SUCCESS(af_release_array(arr));
    ASSERT_SUCCESS(af_release_copy_engine(ce));
}

class CopyEngineExternal
    : public ::testing::TestWithParam<tuple<af_copy_engine_type, dim4, int> > {
    void SetUp() override {
        ce_params params = GetParam();
        ce               = 0;
        done             = false;

        af_copy_engine_properties properties[] = {
            AF_COPY_ENGINE_PROPERTY_MAX_SIZE,
            static_cast<af_copy_engine_properties>(params.max_batch_size),
            AF_COPY_ENGINE_PROPERTY_END};

        dim_t* size = params.shape.get();
        ASSERT_SUCCESS(af_create_copy_engine(&ce, params.type,
                                             params.shape.ndims(), size, f32,
                                             nullptr, properties, nullptr));
    }
    void TearDown() override { ASSERT_SUCCESS(af_release_copy_engine(ce)); }

   public:
    void async_copy_engine_function() {
        af_copy_engine_result result  = AF_COPY_ENGINE_RESULT_SUCCESS;
        ce_params params              = GetParam();
        pair<float*, af_event> ptr[2] = {{nullptr, nullptr},
                                         {nullptr, nullptr}};
        ASSERT_SUCCESS(af_alloc_pinned(
            (void**)&ptr[0].first, params.shape.elements() * sizeof(float)));
        ASSERT_SUCCESS(af_alloc_pinned(
            (void**)&ptr[1].first, params.shape.elements() * sizeof(float)));
        ASSERT_SUCCESS(af_create_event(&ptr[0].second));
        ASSERT_SUCCESS(af_create_event(&ptr[1].second));
        int size = params.shape.elements();
        while (val < 100000 && !done) {
            std::swap(ptr[0], ptr[1]);
            val++;
            ASSERT_SUCCESS(af_block_event(ptr[0].second));
            float* p = ptr[0].first;
            for (int ii = 0; ii < size; ii++) { p[ii] = val; }

            af_err err = AF_SUCCESS;
            do {
                err = af_upload_copy_engine(ce, ptr[0].first,
                                            size * sizeof(float), result, 0,
                                            nullptr, ptr[0].second);
            } while (err == AF_ERR_COPY_ENGINE_TIMEOUT && !done);
            ASSERT_SUCCESS(err);
        }
        done = true;
        ASSERT_SUCCESS(af_free_pinned(ptr[0].first));
        ASSERT_SUCCESS(af_free_pinned(ptr[1].first));
        ASSERT_SUCCESS(af_delete_event(ptr[0].second));
        ASSERT_SUCCESS(af_delete_event(ptr[1].second));
        result = static_cast<af_copy_engine_result>(AF_COPY_ENGINE_RESULT_SKIP |
                                                    AF_COPY_ENGINE_RESULT_DONE);
        ASSERT_SUCCESS(
            af_upload_copy_engine(ce, nullptr, 0, result, 0, nullptr, nullptr));
    }

    af_copy_engine ce;
    atomic<bool> done;
    int val = 0;
};

INSTANTIATE_TEST_CASE_P(
    CopyEngineFunction, CopyEngineExternal,
    ::testing::Combine(
        ::testing::Values(AF_COPY_ENGINE_LATEST, AF_COPY_ENGINE_QUEUE,
                          AF_COPY_ENGINE_BATCH_VARIABLE,
                          AF_COPY_ENGINE_BATCH_FIXED),
        ::testing::Values(dim4(100), dim4(100, 100), dim4(10, 10, 10)),
        ::testing::Values(1, 2, 4, 8, 16, 256)),
    tupleToTestName);

TEST(CopyEngineExternal, LoopCreateAndDelete) {
    af_array arr                           = 0;
    af_copy_engine ce                      = 0;
    af_copy_engine_properties properties[] = {
        AF_COPY_ENGINE_PROPERTY_MAX_SIZE,
        static_cast<af_copy_engine_properties>(10),
        AF_COPY_ENGINE_PROPERTY_END};

    my_user_data data{0, 10};
    dim_t size[] = {10};
    ASSERT_SUCCESS(af_create_copy_engine(&ce, AF_COPY_ENGINE_BATCH_VARIABLE, 1,
                                         size, f32, nullptr, properties,
                                         &data));

    ASSERT_SUCCESS(af_release_copy_engine(ce));
}

TEST_P(CopyEngineExternal, LoopSyncBatch) {
    ce_params params = GetParam();

    af_array a       = 0;
    int slice_values = 0;

    thread external_thread_loop(&CopyEngineExternal::async_copy_engine_function,
                                this);

    for (int i = 0; i < 100; i++) {
        af_copy_engine_status state;
        ASSERT_SUCCESS(af_get_copy_engine_status(&state, ce));
        if (state == AF_COPY_ENGINE_STATUS_DONE) { break; }

        if (params.type == AF_COPY_ENGINE_LATEST) {
            slice_values = val;
            ASSERT_SUCCESS(af_next_array_copy_engine(ce, &a)) << (done = true);
        } else {
            ASSERT_SUCCESS(af_next_array_copy_engine(ce, &a));
        }

        unsigned int ndims = 0;
        if (!a) { break; }

        ASSERT_SUCCESS(af_get_numdims(&ndims, a));

        dim_t dims[4];
        ASSERT_SUCCESS(af_get_dims(&dims[0], &dims[1], &dims[2], &dims[3], a));

        switch (params.type) {
            case AF_COPY_ENGINE_QUEUE:
            case AF_COPY_ENGINE_LATEST:
                ASSERT_EQ(params.shape, dim4(4, dims));
                break;
            case AF_COPY_ENGINE_BATCH_FIXED:
                ASSERT_EQ(params.max_batch_size, dims[params.shape.ndims()]);
                break;
            case AF_COPY_ENGINE_BATCH_VARIABLE:
                ASSERT_LE(dims[params.shape.ndims()], params.max_batch_size);
                break;
        }

        af_array out  = 0;
        int use_count = 0;
        ASSERT_SUCCESS(af_add(&out, a, a, false));
        for (int i = 0; i < 5; i++) {
            af_array oout = 0;
            ASSERT_SUCCESS(af_add(&oout, out, a, false));
            ASSERT_SUCCESS(af_release_array(out));
            af_array oout2 = 0;
            ASSERT_SUCCESS(af_matmul(&oout2, a, a, AF_MAT_NONE, AF_MAT_TRANS));
            ASSERT_SUCCESS(af_release_array(oout2));
            out = oout;
        }
        ASSERT_SUCCESS(af_eval(out));
        ASSERT_SUCCESS(af_release_array(out));

        size_t elements = 0;
        if (params.type == AF_COPY_ENGINE_BATCH_VARIABLE) {
            elements = dims[0] * dims[1] * dims[2] * dims[3];
        } else {
            elements = dims[params.shape.ndims()] * params.shape.elements();
        }

        vector<float> gold_data(elements);
        elements       = params.shape.elements();
        int num_slices = dims[params.shape.ndims()];

        if (params.type != AF_COPY_ENGINE_LATEST) {
            for (int ii = 0; ii < num_slices; ii++) {
                if (params.type != AF_COPY_ENGINE_LATEST) { slice_values++; }
                for (int jj = 0; jj < elements; jj++) {
                    gold_data[elements * ii + jj] = slice_values;
                }
            }

            EXPECT_VEC_ARRAY_EQ(gold_data, dim4(ndims, dims), a);
        }
        ASSERT_SUCCESS(af_release_array(a));
        a = 0;
    }
    ASSERT_SUCCESS(af_stop_copy_engine(ce));
    done = true;
    external_thread_loop.join();
}
