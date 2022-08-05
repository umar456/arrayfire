/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#include <events.hpp>

#include <common/Event.hpp>
#include <common/err_common.hpp>
#include <platform.hpp>
#include <af/device.h>
#include <af/event.h>

using common::Event;
using detail::getActiveDeviceId;
using detail::getQueueHandle;
using std::make_unique;

Event &getEvent(af_event handle) {
    Event &event = *static_cast<Event *>(handle);
    return event;
}

af_event getHandle(Event &event) { return static_cast<af_event>(&event); }

af_event createEventHandle() {
    // Ensure that the default queue is initialized
    auto e = make_unique<Event>();
    POLICY_ASSERT(e->create());
    return getHandle(*e.release());
}

void markEventOnActiveQueue(af_event eventHandle) {
    Event &event = getEvent(eventHandle);
    // Use the currently-active queue
    POLICY_ASSERT(event.mark(getQueueHandle(getActiveDeviceId())));
}

void enqueueWaitOnActiveQueue(af_event eventHandle) {
    Event &event = getEvent(eventHandle);
    // Use the currently-active queue
    POLICY_ASSERT(event.enqueueWait(getQueueHandle(getActiveDeviceId())));
}

void block(af_event eventHandle) {
    Event &event = getEvent(eventHandle);
    POLICY_ASSERT(event.block());
}

af_event createAndMarkEvent() {
    af_event handle = createEventHandle();
    markEventOnActiveQueue(handle);
    return handle;
}

af_err af_create_event(af_event *handle) {
    try {
        AF_CHECK(af_init());
        *handle = createEventHandle();
    }
    CATCHALL;

    return AF_SUCCESS;
}

af_err af_delete_event(af_event handle) {
    try {
        delete &getEvent(handle);
    }
    CATCHALL;

    return AF_SUCCESS;
}

af_err af_mark_event(const af_event handle) {
    try {
        markEventOnActiveQueue(handle);
    }
    CATCHALL;

    return AF_SUCCESS;
}

af_err af_enqueue_wait_event(const af_event handle) {
    try {
        enqueueWaitOnActiveQueue(handle);
    }
    CATCHALL;

    return AF_SUCCESS;
}

af_err af_block_event(const af_event handle) {
    try {
        block(handle);
    }
    CATCHALL;

    return AF_SUCCESS;
}
