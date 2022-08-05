/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once

#include <Policy.hpp>
#include <backend.hpp>
#include <common/Event.hpp>
#include <af/event.h>

/// Convert a reference to an Event object to an af_event object
///
/// \param[in] event the Event object
/// \returns the af_event handle of the \param event object
af_event getHandle(common::Event& event);

/// Convert a handle to an af_event object to an Event object
///
/// \param[in] eventHandle the af_event handle
/// \returns the Event object of the \param eventHandle handle
common::Event& getEvent(af_event eventHandle);

/// Creates a new af_event handle
///
/// \returns A new af_event handle
af_event createEventHandle();

/// Insert the event object on the currently active queue.
///
/// \param[in] eventHandle  the event handle that will be inserted into the
/// current queue.
void markEventOnActiveQueue(af_event eventHandle);

/// Wait for event on the active queue
///
/// \param[in] eventHandle The event to wait on the active queue
void enqueueWaitOnActiveQueue(af_event eventHandle);

/// Block the calling thread for a particular event
void block(af_event eventHandle);

/// Create a new event and mark it and return its handle
af_event createAndMarkEvent();
