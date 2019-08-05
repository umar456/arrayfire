/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once

#include <Event.hpp>
#include <backend.hpp>
#include <af/event.h>

using Event = detail::Event;

af_event getHandle(const Event& event);

Event& getEvent(const af_event eventHandle);
