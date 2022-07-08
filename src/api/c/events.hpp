/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once

#include <common/Event.hpp>
#include <Policy.hpp>
#include <backend.hpp>
#include <af/event.h>

af_event getHandle(common::Event& event);

common::Event& getEvent(af_event& eventHandle);

const common::Event& getEvent(const af_event& eventHandle);

af_event createEvent();

void markEventOnActiveQueue(af_event eventHandle);

void enqueueWaitOnActiveQueue(af_event eventHandle);

void block(af_event eventHandle);

af_event createAndMarkEvent();
