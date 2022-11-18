/*******************************************************
 * Copyright (c) 2022, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once

#include <CL/sycl.hpp>
#include <af/oneapi.h>

#include <memory>
#include <string>

// Forward declarations
namespace spdlog {
class logger;
}

namespace graphics {
class ForgeManager;
}

namespace common {
namespace memory {
class MemoryManagerBase;
}
}  // namespace common

using common::memory::MemoryManagerBase;

namespace oneapi {

// Forward declarations
class GraphicsResourceManager;
class PlanCache;  // clfft

bool verify_present(const std::string& pname, const std::string ref);

int getBackend();

std::string getDeviceInfo() noexcept;

int getDeviceCount() noexcept;

void init();

unsigned getActiveDeviceId();

int& getMaxJitSize();

const sycl::context& getContext();

sycl::queue& getQueue();

/// Return a handle to the queue for the device.
///
/// \param[in] device The device of the returned queue
/// \returns The handle to the queue
sycl::queue* getQueueHandle(int device);

const sycl::device& getDevice(int id = -1);

size_t getDeviceMemorySize(int device);

size_t getHostMemorySize();

inline unsigned getMemoryBusWidth(const sycl::device& device) {
    return device.get_info<sycl::info::device::global_mem_cache_line_size>();
}

// OCL only reports on L1 cache, so we have to estimate the L2 Cache
// size. From studying many GPU cards, it is noticed that their is a
// direct correlation between Cache line and L2 Cache size:
//      - 16KB L2 Cache for each bit in Cache line.
//        Example: RTX3070 (4096KB of L2 Cache, 256Bit of Cache
//        line)
//                   --> 256*16KB = 4096KB
//      - This is also valid for all AMD GPU's
//      - Exceptions
//          * GTX10XX series have 8KB per bit of cache line
//          * iGPU (64bit cacheline) have 5KB per bit of cache line
inline size_t getL2CacheSize(const sycl::device& device) {
    const unsigned cacheLine{getMemoryBusWidth(device)};
    return cacheLine * 1024ULL *
           (cacheLine == 64 ? 5
            : device.get_info<sycl::info::device::name>().find(
                  "GTX 10") == std::string::npos
                ? 16
                : 8);
}

inline unsigned getComputeUnits(const sycl::device& device) {
    return device.get_info<sycl::info::device::max_compute_units>();
}

// maximum nr of threads the device really can run in parallel, without
// scheduling
inline unsigned getMaxParallelThreads(const sycl::device& device) {
    return getComputeUnits(device) * 2048;
}

// sycl::device::is_cpu,is_gpu,is_accelerator
sycl::info::device_type getDeviceType();

bool isHostUnifiedMemory(const sycl::device& device);

bool OneAPICPUOffload(bool forceOffloadOSX = true);

bool isGLSharingSupported();

bool isDoubleSupported(unsigned device);

// Returns true if 16-bit precision floats are supported by the device
bool isHalfSupported(unsigned device);

void devprop(char* d_name, char* d_platform, char* d_toolkit, char* d_compute);

std::string getPlatformName(const sycl::device& device);

int setDevice(int device);

void addDeviceContext(sycl::device dev, sycl::context ctx, sycl::queue que);

void setDeviceContext(sycl::device dev, sycl::context ctx);

void removeDeviceContext(sycl::device dev, sycl::context ctx);

void sync(int device);

bool synchronize_calls();

int getActiveDeviceType();

int getActivePlatform();

bool& evalFlag();

MemoryManagerBase& memoryManager();

void setMemoryManager(std::unique_ptr<MemoryManagerBase> mgr);

void resetMemoryManager();

MemoryManagerBase& pinnedMemoryManager();

void setMemoryManagerPinned(std::unique_ptr<MemoryManagerBase> mgr);

void resetMemoryManagerPinned();

graphics::ForgeManager& forgeManager();

GraphicsResourceManager& interopManager();

// afcl::platform getPlatformEnum(cl::Device dev);

void setActiveContext(int device);

}  // namespace oneapi
