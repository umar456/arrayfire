

namespace af {
  class Program {

    CUmodule module;
    void *cubin;
    size_t cubinSize;


    Program(std::string name, std::string source) : program_(0) {
      nvrtcProgram program_;
      nvrtcCreateProgram(&program_, source.c_str(), name.c_str(), 0, nullptr, nullptr);
      auto dev = getDeviceProp(getActiveDeviceId());


      array<char, 64> arch;
      snprintf(arch.data(), arch.size(), "--gpu-architecture=compute_%d%d", dev.major, dev.minor);
      const char* compiler_options[] = {arch.data(),
#if !(defined(NDEBUG) || defined(__aarch64__) || defined(__LP64__))
                                        "--device-debug",
                                        "--generate-line-info"
#endif
      };

      int num_options = std::extent<decltype(compiler_options)>::value;
      NVRTC_CHECK(nvrtcCompileProgram(prog, num_options, compiler_options));

      NVRTC_CHECK(nvrtcGetPTXSize(prog, &ptx_size));
      std::string ptx;
      ptx.resize(ptx_size);
      NVRTC_CHECK(nvrtcGetPTX(prog, ptx.data()));
      NVRTC_CHECK(nvrtcDestroyProgram(&prog));

      CUlinkState linkState;
      CUjit_option linkOptions[] = {
                                    CU_JIT_INFO_LOG_BUFFER,
                                    CU_JIT_INFO_LOG_BUFFER_SIZE_BYTES,
                                    CU_JIT_ERROR_LOG_BUFFER,
                                    CU_JIT_ERROR_LOG_BUFFER_SIZE_BYTES,
                                    CU_JIT_LOG_VERBOSE
      };

      void *linkOptionValues[] = {
                                  linkInfo,
                                  reinterpret_cast<void*>(linkLogSize),
                                  linkError,
                                  reinterpret_cast<void*>(linkLogSize),
                                  reinterpret_cast<void*>(1)
      };


      CU_LINK_CHECK(cuLinkCreate(5, linkOptions, linkOptionValues, &linkState));
      CU_LINK_CHECK(cuLinkAddData(linkState, CU_JIT_INPUT_PTX, (void*)ptx.data(),
                                  ptx.size(), ker_name, 0, NULL, NULL));

      CUfunction kernel;
      CU_LINK_CHECK(cuLinkComplete(linkState, &cubin, &cubinSize));
      CU_CHECK(cuModuleLoadDataEx(&module, cubin, 0, 0, 0));
      CU_CHECK(cuModuleGetFunction(&kernel, module, ker_name));
      CU_LINK_CHECK(cuLinkDestroy(linkState));
      kc_entry_t entry = {module, kernel};
      return entry;
    }
  }
}
