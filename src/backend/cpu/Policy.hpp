/*******************************************************
 * Copyright (c) 2022, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/
#pragma once

#include <queue.hpp>

#include <common/err_common.hpp>

#include <nonstd/span.hpp>
#include <cstring>

namespace cpu {

class Policy {
   public:
    using EventType  = queue_event *;
    using QueueType  = cpu::queue *;
    using ErrorType  = int;
    using MemoryType = void *;

    static ErrorType createQueue(QueueType *queue) {
        *queue = new cpu::queue();
        return 0;
    }

    static int destroyQueue(QueueType e) noexcept {
        delete e;
        return 0;
    }

    static void policy_assert(ErrorType err, const char *file, int line) {
        if (err != 0) {
            char err_msg[1024];
            snprintf(err_msg, sizeof(err_msg), "%s:%d: CPU Policy Error %d",
                     file, line, err);
            throw AfError("policy_assert", file, line, (const char *)err_msg,
                          AF_ERR_INTERNAL, boost::stacktrace::stacktrace());
        }
    }
    static ErrorType memcpyToDevice(QueueType &queue, MemoryType gpu_mem,
                                    MemoryType pinned_mem, size_t write_bytes,
                                    size_t offset_bytes,
                                    nonstd::span<EventType> wait_events,
                                    EventType &write_event) {
        if (!wait_events.empty())
            waitForEvents(queue, wait_events.size(), wait_events.data());
        queue->enqueue(memcpy, ((char *)gpu_mem + offset_bytes), pinned_mem,
                       write_bytes);
        if (write_event) write_event->mark(*queue);
        return 0;
    }

    static MemoryType allocateMemory(size_t bytes) {
        return memAlloc<char>(bytes).release();
    }

    static int createEvent(EventType *e) noexcept {
        *e = new cpu::queue_event;
        (*e)->create();
        return 0;
    }

    static int markEvent(QueueType stream, EventType e) noexcept {
        e->mark(*stream);
        return 0;
    }

    static int waitForEvents(QueueType stream, int nEvents,
                             EventType *events) noexcept {
        int err = 0;
        for (int i = 0; i < nEvents; i++) {
            err = events[i]->wait(*stream);
            if (err) return err;
        }
        return err;
    }

    static int syncForEvents(int nEvents, EventType *events) noexcept {
        int err = 0;
        for (int i = 0; i < nEvents; i++) {
            err = events[i]->sync();
            if (err) return err;
        }
        return 0;
    }

    static int destroyEvent(EventType e) noexcept {
        delete e;
        return 0;
    }
};
}  // namespace cpu

#define POLICY_ASSERT(fn) cpu::Policy::policy_assert((fn), __FILE__, __LINE__)
