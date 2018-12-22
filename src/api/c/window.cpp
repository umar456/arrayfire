/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/


#include <af/graphics.h>
#include <af/algorithm.h>

#include <common/graphics_common.hpp>
#include <common/err_common.hpp>
#include <backend.hpp>
#include <platform.hpp>

using af::dim4;
using namespace detail;
using namespace graphics;

af_err af_create_window(af_window *out, const int width, const int height, const char* const title)
{
    try {
        ForgeManager& fgMngr = forgeManager();
        fg_window mainWnd    = fgMngr.getMainWindow();

        if (mainWnd == 0) {
            AF_ERROR("OpenGL context creation failed", AF_ERR_INTERNAL);
        }

        fg_window temp = nullptr;

        FG_CHECK(forgePlugin().fg_create_window(&temp, width, height, title, mainWnd, false));

        fgMngr.setWindowChartGrid(temp, 1, 1);

        std::swap(*out, temp);
    }
    CATCHALL;
    return AF_SUCCESS;
}

af_err af_set_position(const af_window wind, const unsigned x, const unsigned y)
{
    try {
        if(wind == 0) {
            AF_ERROR("Not a valid window", AF_ERR_INTERNAL);
        }
        FG_CHECK(forgePlugin().fg_set_window_position(wind, x, y));
    }
    CATCHALL;
    return AF_SUCCESS;
}

af_err af_set_title(const af_window wind, const char* const title)
{
    try {
        if(wind == 0) {
            AF_ERROR("Not a valid window", AF_ERR_INTERNAL);
        }
        FG_CHECK(forgePlugin().fg_set_window_title(wind, title));
    }
    CATCHALL;
    return AF_SUCCESS;
}

af_err af_set_size(const af_window wind, const unsigned w, const unsigned h)
{
    try {
        if(wind == 0) {
            AF_ERROR("Not a valid window", AF_ERR_INTERNAL);
        }
        FG_CHECK(forgePlugin().fg_set_window_size(wind, w, h));
    }
    CATCHALL;
    return AF_SUCCESS;
}

af_err af_grid(const af_window wind, const int rows, const int cols)
{
    try {
        if(wind == 0) {
            AF_ERROR("Not a valid window", AF_ERR_INTERNAL);
        }
        forgeManager().setWindowChartGrid(wind, rows, cols);
    }
    CATCHALL;
    return AF_SUCCESS;
}

af_err af_set_axes_limits_compute(const af_window window,
                                  const af_array x, const af_array y, const af_array z,
                                  const bool exact, const af_cell* const props)
{
    try {
        if(window == 0) {
            AF_ERROR("Not a valid window", AF_ERR_INTERNAL);
        }

        ForgeManager& fgMngr = forgeManager();

        fg_chart chart = NULL;

        fg_chart_type ctype = (z ? FG_CHART_3D : FG_CHART_2D);

        if (props->col > -1 && props->row > -1)
            chart = fgMngr.getChart(window, props->row, props->col, ctype);
        else
            chart = fgMngr.getChart(window, 0, 0, ctype);

        double xmin = -1, xmax = 1;
        double ymin = -1, ymax = 1;
        double zmin = -1, zmax = 1;
        AF_CHECK(af_min_all(&xmin, NULL, x));
        AF_CHECK(af_max_all(&xmax, NULL, x));
        AF_CHECK(af_min_all(&ymin, NULL, y));
        AF_CHECK(af_max_all(&ymax, NULL, y));

        if(ctype == FG_CHART_3D) {
            AF_CHECK(af_min_all(&zmin, NULL, z));
            AF_CHECK(af_max_all(&zmax, NULL, z));
        }

        if(!exact) {
            xmin = step_round(xmin, false);
            xmax = step_round(xmax, true );
            ymin = step_round(ymin, false);
            ymax = step_round(ymax, true );
            zmin = step_round(zmin, false);
            zmax = step_round(zmax, true );
        }

        fgMngr.setChartAxesOverride(chart);
        FG_CHECK(forgePlugin().fg_set_chart_axes_limits(chart, xmin, xmax,
                                          ymin, ymax, zmin, zmax));
    }
    CATCHALL;
    return AF_SUCCESS;
}

af_err af_set_axes_limits_2d(const af_window window,
                             const float xmin, const float xmax,
                             const float ymin, const float ymax,
                             const bool exact, const af_cell* const props)
{
    try {
        if(window == 0) {
            AF_ERROR("Not a valid window", AF_ERR_INTERNAL);
        }

        ForgeManager& fgMngr = forgeManager();

        fg_chart chart = NULL;
        // The ctype here below doesn't really matter as it is only fetching
        // the chart. It will not set it.
        // If this is actually being done, then it is extremely bad.
        fg_chart_type ctype = FG_CHART_2D;

        if (props->col > -1 && props->row > -1)
            chart = fgMngr.getChart(window, props->row, props->col, ctype);
        else
            chart = fgMngr.getChart(window, 0, 0, ctype);

        float _xmin = xmin;
        float _xmax = xmax;
        float _ymin = ymin;
        float _ymax = ymax;
        if(!exact) {
            _xmin = step_round(_xmin, false);
            _xmax = step_round(_xmax, true );
            _ymin = step_round(_ymin, false);
            _ymax = step_round(_ymax, true );
        }

        fgMngr.setChartAxesOverride(chart);
        FG_CHECK(forgePlugin().fg_set_chart_axes_limits(chart, _xmin, _xmax,
                                          _ymin, _ymax, 0.0f, 0.0f));
    }
    CATCHALL;
    return AF_SUCCESS;
}

af_err af_set_axes_limits_3d(const af_window window,
                             const float xmin, const float xmax,
                             const float ymin, const float ymax,
                             const float zmin, const float zmax,
                             const bool exact, const af_cell* const props)
{
    try {
        if(window == 0) {
            AF_ERROR("Not a valid window", AF_ERR_INTERNAL);
        }

        ForgeManager& fgMngr = forgeManager();

        fg_chart chart = NULL;
        // The ctype here below doesn't really matter as it is only fetching
        // the chart. It will not set it.
        // If this is actually being done, then it is extremely bad.
        fg_chart_type ctype = FG_CHART_3D;

        if (props->col > -1 && props->row > -1)
            chart = fgMngr.getChart(window, props->row, props->col, ctype);
        else
            chart = fgMngr.getChart(window, 0, 0, ctype);

        float _xmin = xmin;
        float _xmax = xmax;
        float _ymin = ymin;
        float _ymax = ymax;
        float _zmin = zmin;
        float _zmax = zmax;
        if(!exact) {
            _xmin = step_round(_xmin, false);
            _xmax = step_round(_xmax, true );
            _ymin = step_round(_ymin, false);
            _ymax = step_round(_ymax, true );
            _zmin = step_round(_zmin, false);
            _zmax = step_round(_zmax, true );
        }

        fgMngr.setChartAxesOverride(chart);
        FG_CHECK(forgePlugin().fg_set_chart_axes_limits(chart, _xmin, _xmax,
                                          _ymin, _ymax, _zmin, _zmax));
    }
    CATCHALL;
    return AF_SUCCESS;
}

af_err af_set_axes_titles(const af_window window,
                          const char * const xtitle,
                          const char * const ytitle,
                          const char * const ztitle,
                          const af_cell* const props)
{
    try {
        if(window == 0) {
            AF_ERROR("Not a valid window", AF_ERR_INTERNAL);
        }

        ForgeManager& fgMngr = forgeManager();

        fg_chart chart = NULL;

        fg_chart_type ctype = (ztitle ? FG_CHART_3D : FG_CHART_2D);

        if (props->col > -1 && props->row > -1)
            chart = fgMngr.getChart(window, props->row, props->col, ctype);
        else
            chart = fgMngr.getChart(window, 0, 0, ctype);

        FG_CHECK(forgePlugin().fg_set_chart_axes_titles(chart, xtitle, ytitle, ztitle));
    }
    CATCHALL;
    return AF_SUCCESS;
}

af_err af_show(const af_window wind)
{
    try {
        if(wind == 0) {
            AF_ERROR("Not a valid window", AF_ERR_INTERNAL);
        }
        FG_CHECK(forgePlugin().fg_swap_window_buffers(wind));
    }
    CATCHALL;
    return AF_SUCCESS;
}

af_err af_is_window_closed(bool *out, const af_window wind)
{
    try {
        if(wind == 0) {
            AF_ERROR("Not a valid window", AF_ERR_INTERNAL);
        }
        FG_CHECK(forgePlugin().fg_close_window(out, wind));
    }
    CATCHALL;
    return AF_SUCCESS;
}

af_err af_set_visibility(const af_window wind, const bool is_visible)
{
    try {
        if(wind == 0) {
            AF_ERROR("Not a valid window", AF_ERR_INTERNAL);
        }
        if (is_visible) {
            FG_CHECK(forgePlugin().fg_show_window(wind));
        } else {
            FG_CHECK(forgePlugin().fg_hide_window(wind));
        }
    }
    CATCHALL;
    return AF_SUCCESS;
}

af_err af_destroy_window(const af_window wind)
{
    try {
        if(wind == 0) {
            AF_ERROR("Not a valid window", AF_ERR_INTERNAL);
        }
        forgeManager().setWindowChartGrid(wind, 0, 0);
        FG_CHECK(forgePlugin().fg_release_window(wind));
    }
    CATCHALL;
    return AF_SUCCESS;
}
