/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#include <memory.hpp>

#include <common/Logger.hpp>
#include <common/MemoryManagerImpl.hpp>
#include <common/dispatch.hpp>
#include <common/util.hpp>
#include <cuda.h>
#include <cuda_runtime.h>
#include <cuda_runtime_api.h>
#include <err_cuda.hpp>
#include <platform.hpp>
#include <spdlog/spdlog.h>
#include <types.hpp>

#include <execinfo.h>

#include <mutex>
#include <map>

template class common::MemoryManager<cuda::MemoryManager>;
template class common::MemoryManager<cuda::MemoryManagerPinned>;

#ifndef AF_MEM_DEBUG
#define AF_MEM_DEBUG 0
#endif

#ifndef AF_CUDA_MEM_DEBUG
#define AF_CUDA_MEM_DEBUG 0
#endif

using common::bytesToString;

using std::function;
using std::lock_guard;
using std::recursive_mutex;
using std::unique_ptr;

namespace cuda {
void setMemStepSize(size_t step_bytes) {
    memoryManager().setMemStepSize(step_bytes);
}

size_t getMemStepSize(void) { return memoryManager().getMemStepSize(); }

size_t getMaxBytes() { return memoryManager().getMaxBytes(); }

unsigned getMaxBuffers() { return memoryManager().getMaxBuffers(); }

void garbageCollect() { memoryManager().garbageCollect(); }

void printMemInfo(const char *msg, const int device) {
    memoryManager().printInfo(msg, device);
}

  static const int BT_BUF_SIZE = 100;


  std::map<void*, std::tuple<string, size_t>> alloc_backtraces;

  template<typename... ARGS>
  std::string
  bt(std::string fmt, ARGS... args)
  {
    int j, nptrs;
    void *buffer[BT_BUF_SIZE];
    char **strings;

    nptrs = backtrace(buffer, BT_BUF_SIZE);
    auto l = spdlog::get("mem");
    l->trace(fmt.c_str(), args...);

    /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
       would produce similar output to the following: */

    strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL) {
      perror("backtrace_symbols");
      exit(EXIT_FAILURE);
    }

    string backtrace_string;
    for (j = 1; j < nptrs; j++) {
      backtrace_string += strings[j];
      backtrace_string += "\n";
    }

    free(strings);
    return backtrace_string;
  }

template<typename T>
uptr<T> memAlloc(const size_t &elements) {
    size_t size = elements * sizeof(T);
    auto out = uptr<T>(static_cast<T *>(memoryManager().alloc(size, false)),
                   memFree<T>);

    string bt_str = bt("memAlloc: {} {}", (void*)out.get(), size);
    alloc_backtraces[out.get()] = make_tuple(bt_str, size);
    puts(bt_str.c_str());
    return out;
}

void *memAllocUser(const size_t &bytes) {
    return memoryManager().alloc(bytes, true);
}
template<typename T>
void memFree(T *ptr) {
    puts(bt("memFree: {}", (void*)ptr).c_str());
    alloc_backtraces.erase((void*)ptr);

    memoryManager().unlock((void *)ptr, false);
}

void memFreeUser(void *ptr) { memoryManager().unlock((void *)ptr, true); }

void memLock(const void *ptr) { memoryManager().userLock((void *)ptr); }

void memUnlock(const void *ptr) { memoryManager().userUnlock((void *)ptr); }

bool isLocked(const void *ptr) {
    return memoryManager().isUserLocked((void *)ptr);
}

void deviceMemoryInfo(size_t *alloc_bytes, size_t *alloc_buffers,
                      size_t *lock_bytes, size_t *lock_buffers) {
    memoryManager().bufferInfo(alloc_bytes, alloc_buffers, lock_bytes,
                               lock_buffers);
}

template<typename T>
T *pinnedAlloc(const size_t &elements) {
    return (T *)pinnedMemoryManager().alloc(elements * sizeof(T), false);
}

template<typename T>
void pinnedFree(T *ptr) {
    return pinnedMemoryManager().unlock((void *)ptr, false);
}

bool checkMemoryLimit() { return memoryManager().checkMemoryLimit(); }

#define INSTANTIATE(T)                                 \
    template uptr<T> memAlloc(const size_t &elements); \
    template void memFree(T *ptr);                     \
    template T *pinnedAlloc(const size_t &elements);   \
    template void pinnedFree(T *ptr);

INSTANTIATE(float)
INSTANTIATE(cfloat)
INSTANTIATE(double)
INSTANTIATE(cdouble)
INSTANTIATE(int)
INSTANTIATE(uint)
INSTANTIATE(char)
INSTANTIATE(uchar)
INSTANTIATE(intl)
INSTANTIATE(uintl)
INSTANTIATE(short)
INSTANTIATE(ushort)

MemoryManager::MemoryManager()
    : common::MemoryManager<cuda::MemoryManager>(
          getDeviceCount(), common::MAX_BUFFERS,
          AF_MEM_DEBUG || AF_CUDA_MEM_DEBUG) {
    this->setMaxMemorySize();
}

MemoryManager::~MemoryManager() {
    for (int n = 0; n < cuda::getDeviceCount(); n++) {
        try {
            cuda::setDevice(n);
            garbageCollect();
        } catch (AfError err) {
            continue;  // Do not throw any errors while shutting down
        }
    }
}

int MemoryManager::getActiveDeviceId() { return cuda::getActiveDeviceId(); }

size_t MemoryManager::getMaxMemorySize(int id) {
    return cuda::getDeviceMemorySize(id);
}

void *MemoryManager::nativeAlloc(const size_t bytes) {
    void *ptr = NULL;
    CUDA_CHECK(cudaMalloc(&ptr, bytes));
    AF_TRACE("nativeAlloc: {:>7} {}", bytesToString(bytes), ptr);
    return ptr;
}

void MemoryManager::nativeFree(void *ptr) {
    AF_TRACE("nativeFree:          {}", ptr);
    cudaError_t err = cudaFree(ptr);
    if (err != cudaErrorCudartUnloading) { CUDA_CHECK(err); }
}

MemoryManagerPinned::MemoryManagerPinned()
    : common::MemoryManager<MemoryManagerPinned>(
          1, common::MAX_BUFFERS, AF_MEM_DEBUG || AF_CUDA_MEM_DEBUG) {
    this->setMaxMemorySize();
}

MemoryManagerPinned::~MemoryManagerPinned() { garbageCollect(); }

int MemoryManagerPinned::getActiveDeviceId() {
    return 0;  // pinned uses a single vector
}

size_t MemoryManagerPinned::getMaxMemorySize(int id) {
    UNUSED(id);
    return cuda::getHostMemorySize();
}

void *MemoryManagerPinned::nativeAlloc(const size_t bytes) {
    void *ptr;
    CUDA_CHECK(cudaMallocHost(&ptr, bytes));
    AF_TRACE("Pinned::nativeAlloc: {:>7} {}", bytesToString(bytes), ptr);
    return ptr;
}

void MemoryManagerPinned::nativeFree(void *ptr) {
    AF_TRACE("Pinned::nativeFree:          {}", ptr);
    cudaError_t err = cudaFreeHost(ptr);
    if (err != cudaErrorCudartUnloading) { CUDA_CHECK(err); }
}
}  // namespace cuda


AFAPI void clear_ptr_bt() {
  cuda::alloc_backtraces.clear();
}

AFAPI void print_ptr_bt() {
  using std::get;
  puts("PRINTING ALLOCATIONS ==============================================");
  for(auto a : cuda::alloc_backtraces) {
      printf("%p %zu:\n%s\n", a.first, get<1>(a.second), get<0>(a.second).c_str());
  }
  puts("DONE PRINTING ALLOCATIONS ==============================================");
}


