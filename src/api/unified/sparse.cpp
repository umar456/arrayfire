/*******************************************************
 * Copyright (c) 2015, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#include <af/sparse.h>
#include "symbol_manager.hpp"

af_err af_create_sparse_array(af_array *out, const dim_t nRows,
                              const dim_t nCols, const af_array values,
                              const af_array rowIdx, const af_array colIdx,
                              const af_storage stype) {
    CHECK_ARRAYS(values, rowIdx, colIdx);
    return CALL(out, nRows, nCols, values, rowIdx, colIdx, stype);
}

af_err af_create_sparse_array_from_ptr(af_array *out, const dim_t nRows,
                                       const dim_t nCols, const dim_t nNZ,
                                       void *const values, int *const rowIdx,
                                       int *const colIdx, const af_dtype type,
                                       const af_storage stype,
                                       const af_source source) {
    return CALL(out, nRows, nCols, nNZ, values, rowIdx, colIdx, type, stype,
                source);
}

af_err af_create_sparse_array_from_dense(af_array *out, const af_array in,
                                         const af_storage stype) {
    CHECK_ARRAYS(in);
    return CALL(out, in, stype);
}

af_err af_sparse_convert_to(af_array *out, const af_array in,
                            const af_storage destStorage) {
    CHECK_ARRAYS(in);
    return CALL(out, in, destStorage);
}

af_err af_sparse_to_dense(af_array *out, const af_array in) {
    CHECK_ARRAYS(in);
    return CALL(out, in);
}

af_err af_sparse_get_info(af_array *values, af_array *rowIdx, af_array *colIdx,
                          af_storage *stype, const af_array in) {
    CHECK_ARRAYS(in);
    return CALL(values, rowIdx, colIdx, stype, in);
}

af_err af_sparse_get_values(af_array *out, const af_array in) {
    CHECK_ARRAYS(in);
    return CALL(out, in);
}

af_err af_sparse_get_row_idx(af_array *out, const af_array in) {
    CHECK_ARRAYS(in);
    return CALL(out, in);
}

af_err af_sparse_get_col_idx(af_array *out, const af_array in) {
    CHECK_ARRAYS(in);
    return CALL(out, in);
}

af_err af_sparse_get_nnz(dim_t *out, const af_array in) {
    CHECK_ARRAYS(in);
    return CALL(out, in);
}

af_err af_sparse_get_storage(af_storage *out, const af_array in) {
    CHECK_ARRAYS(in);
    return CALL(out, in);
}
