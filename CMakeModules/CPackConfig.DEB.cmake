
##
# Debian package
##
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEB_COMPONENT_INSTALL ON)
#set(CMAKE_INSTALL_RPATH /usr/lib;${ArrayFire_BUILD_DIR}/third_party/forge/lib)
set(CPACK_DEBIAN_DEBUGINFO_PACKAGE OFF)
set(CPACK_DEBIAN_PACKAGE_DEBUG ON)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS_POLICY ">=")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE http://www.arrayfire.com)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
#set(CPACK_DEBIAN_PACKAGE_EPOCH 1)

set(CPACK_DEBIAN_PACKAGE_NAME "arrayfire")

macro(af_deb_component)
  cmake_parse_arguments(RC "USE_SHLIBDEPS;ADD_POSTINST" "COMPONENT;PACKAGE_NAME;FILE_NAME;PROVIDES;REPLACES" "REQUIRES;OPTIONAL" ${ARGN})

  string(REPLACE ";" ", " REQ "${RC_REQUIRES}")
  string(REPLACE ";" ", " OPT "${RC_OPTIONAL}")
  string(REPLACE ";" ", " PROVIDES "${RC_PROVIDES}")
  string(TOUPPER ${RC_COMPONENT} COMPONENT_UPPER)

  if(RC_PACKAGE_NAME)
    set(CPACK_DEBIAN_${COMPONENT_UPPER}_PACKAGE_NAME "${RC_PACKAGE_NAME}")
  endif()

  if(RC_FILE_NAME)
    set(CPACK_DEBIAN_${COMPONENT_UPPER}_FILE_NAME "${RC_FILE_NAME}")
  endif()

  set(CPACK_DEBIAN_${COMPONENT_UPPER}_PACKAGE_SUGGESTS ${OPT})
  if(REQ)
    set(CPACK_DEBIAN_${COMPONENT_UPPER}_PACKAGE_DEPENDS "${REQ}")
  endif()
  if(RC_USE_SHLIBDEPS)
    set(CPACK_DEBIAN_${COMPONENT_UPPER}_PACKAGE_SHLIBDEPS ON)
  else()
    set(CPACK_DEBIAN_${COMPONENT_UPPER}_PACKAGE_SHLIBDEPS OFF)
  endif()
  if(RC_PROVIDES)
    set(CPACK_DEBIAN_${COMPONENT_UPPER}_PACKAGE_PROVIDES ${PROVIDES})
  endif()
  if(RC_REPLACES)
    set(CPACK_DEBIAN_${COMPONENT_UPPER}_PACKAGE_REPLACES ${RC_REPLACES})
    set(CPACK_DEBIAN_${COMPONENT_UPPER}_PACKAGE_CONFLICTS ${RC_REPLACES})
  endif()

  if(RC_ADD_POSTINST)
    set(CPACK_DEBIAN_${COMPONENT_UPPER}_PACKAGE_CONTROL_EXTRA
      "${ArrayFire_SOURCE_DIR}/CMakeModules/debian/postinst")
  endif()
endmacro()

af_deb_component(
  COMPONENT arrayfire
  PACKAGE_NAME arrayfire
  REQUIRES arrayfire-cpu3-dev
           arrayfire-cuda3-dev
           arrayfire-opencl3-dev
           arrayfire-unified3-dev
           arrayfire-examples
           arrayfire-headers
           arrayfire-cmake
           arrayfire-doc
)

# This approach doesn't work because the name is resolved at CMake time and not
# CPack time so the RELEASE number which is a variable set by cpack is not
# included in the file name. Use the CPACK_DEBIAN_ARRAYFIRE_PACKAGE_NAME
# variable to set it from the command line
#message("CPACK_DEBIAN_PACKAGE_RELEASE: ${CPACK_DEBIAN_PACKAGE_RELEASE}")
#if(CPACK_DEBIAN_PACKAGE_RELEASE EQUAL "1")
#  set(CPACK_DEBIAN_ARRAYFIRE_FILE_NAME "arrayfire-${ArrayFire_VERSION}_amd64.deb")
#else()
#  set(CPACK_DEBIAN_ARRAYFIRE_FILE_NAME "arrayfire-${ArrayFire_VERSION}-\${CPACK_DEBIAN_PACKAGE_RELEASE}_amd64.deb")
#endif()
#set(CPACK_DEBIAN_ARRAYFIRE_PACKAGE_NAME "arrayfire")
#set(CPACK_VERBATIM_VARIABLES FALSE)

execute_process(COMMAND lsb_release -cs
  OUTPUT_VARIABLE RELEASE_CODENAME
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

af_deb_component(
  COMPONENT dev
  PACKAGE_NAME arrayfire-dev)

if(USE_CPU_MKL)
  set(cpu_runtime_package_name arrayfire-cpu${ArrayFire_VERSION_MAJOR}-mkl)
  set(cpu_runtime_requirements "intel-mkl-core-rt-2020.0-166, intel-mkl-gnu-rt-2020.0-166")
else()
  set(cpu_runtime_package_name arrayfire-cpu${ArrayFire_VERSION_MAJOR}-openblas)
  set(cpu_runtime_requirements "")
endif()

af_deb_component(
  COMPONENT cpu
  PACKAGE_NAME ${cpu_runtime_package_name}
  REQUIRES ${cpu_runtime_requirements}
  PROVIDES "arrayfire-cpu (= ${ArrayFire_VERSION}), arrayfire-cpu${ArrayFire_VERSION_MAJOR} (= ${ArrayFire_VERSION}), libarrayfire-cpu${ArrayFire_VERSION_MAJOR} (= ${ArrayFire_VERSION})"
  REPLACES "arrayfire-cpu, arrayfire-cpu${ArrayFire_VERSION_MAJOR} (<< ${ArrayFire_VERSION}), libarrayfire-cpu${ArrayFire_VERSION_MAJOR} (<< ${ArrayFire_VERSION})"
  USE_SHLIBDEPS
  ADD_POSTINST
  OPTIONAL forge libfreeimage3)


af_deb_component(
  COMPONENT cpu-dev
  PACKAGE_NAME arrayfire-cpu${ArrayFire_VERSION_MAJOR}-dev
  PROVIDES "arrayfire-cpu-dev (= ${ArrayFire_VERSION}), arrayfire-cpu${ArrayFire_VERSION_MAJOR}-dev (= ${ArrayFire_VERSION}), libarrayfire-cpu-dev (= ${ArrayFire_VERSION})"
  REPLACES "arrayfire-cpu-dev (<< ${ArrayFire_VERSION}), arrayfire-cpu${ArrayFire_VERSION_MAJOR}-dev (<< ${ArrayFire_VERSION}), libarrayfire-cpu3-dev (<< ${ArrayFire_VERSION})"
  REQUIRES "arrayfire-cpu${ArrayFire_VERSION_MAJOR}-openblas (>= ${ArrayFire_VERSION}) | arrayfire-cpu${ArrayFire_VERSION_MAJOR}-mkl (>= ${ArrayFire_VERSION}), arrayfire-headers (>= ${ArrayFire_VERSION}), arrayfire-cmake (>= ${ArrayFire_VERSION})"
  OPTIONAL "cmake (>= 3.0)"
  )

af_deb_component(
  COMPONENT cuda
  PACKAGE_NAME arrayfire-cuda${ArrayFire_VERSION_MAJOR}-cuda-${CUDA_VERSION_MAJOR}-${CUDA_VERSION_MINOR}
  ADD_POSTINST
  REQUIRES "cuda-nvrtc-${CUDA_VERSION_MAJOR}-${CUDA_VERSION_MINOR}, libc6 (>= 2.27), libgcc1 (>= 1:4.0), libstdc++6 (>= 5.2) "
  PROVIDES "arrayfire-cuda (= ${ArrayFire_VERSION}), arrayfire-cuda${ArrayFire_VERSION_MAJOR} (= ${ArrayFire_VERSION}), libarrayfire-cuda${ArrayFire_VERSION_MAJOR} (= ${ArrayFire_VERSION})"
  REPLACES "arrayfire-cuda (<< ${ArrayFire_VERSION}), arrayfire-cuda${ArrayFire_VERSION_MAJOR} (<< ${ArrayFire_VERSION})"
  OPTIONAL forge libfreeimage3)


af_deb_component(
  COMPONENT cuda-dev
  PACKAGE_NAME arrayfire-cuda${ArrayFire_VERSION_MAJOR}-dev
  PROVIDES "arrayfire-cuda-dev (= ${ArrayFire_VERSION}), arrayfire-cuda${ArrayFire_VERSION_MAJOR}-dev (= ${ArrayFire_VERSION}), libarrayfire-cuda-dev (= ${ArrayFire_VERSION})"
  REPLACES "arrayfire-cuda-dev (<< ${ArrayFire_VERSION}), arrayfire-cuda${ArrayFire_VERSION_MAJOR}-dev (<< ${ArrayFire_VERSION})"
  REQUIRES "arrayfire-cuda${ArrayFire_VERSION_MAJOR} (>= ${ArrayFire_VERSION}), arrayfire-headers (>= ${ArrayFire_VERSION}), arrayfire-cmake (>= ${ArrayFire_VERSION})"
  OPTIONAL "cmake (>= 3.0)"
  )

if(USE_OPENCL_MKL)
  set(opencl_runtime_package_name arrayfire-opencl${ArrayFire_VERSION_MAJOR}-mkl)
else()
  set(opencl_runtime_package_name arrayfire-opencl${ArrayFire_VERSION_MAJOR}-openblas)
endif()

af_deb_component(
  COMPONENT opencl
  PACKAGE_NAME ${opencl_runtime_package_name}
  PROVIDES "arrayfire-opencl (= ${ArrayFire_VERSION}), arrayfire-opencl${ArrayFire_VERSION_MAJOR} (= ${ArrayFire_VERSION}), libarrayfire-opencl${ArrayFire_VERSION_MAJOR} (= ${ArrayFire_VERSION})"
  REPLACES "arrayfire-opencl (<< ${ArrayFire_VERSION}), arrayfire-opencl${ArrayFire_VERSION_MAJOR} (<< ${ArrayFire_VERSION}), libarrayfire-opencl${ArrayFire_VERSION_MAJOR} (<< ${ArrayFire_VERSION})"
  REQUIRES ${opencl_distro_dependencies}
  USE_SHLIBDEPS
  ADD_POSTINST
  OPTIONAL forge libfreeimage3)

af_deb_component(
  COMPONENT opencl-dev
  PACKAGE_NAME arrayfire-opencl${ArrayFire_VERSION_MAJOR}-dev
  PROVIDES "arrayfire-opencl-dev (= ${ArrayFire_VERSION}), arrayfire-opencl${ArrayFire_VERSION_MAJOR}-dev (= ${ArrayFire_VERSION}), libarrayfire-opencl-dev (= ${ArrayFire_VERSION})"
  REPLACES "arrayfire-opencl-dev (<< ${ArrayFire_VERSION}), arrayfire-opencl${ArrayFire_VERSION_MAJOR}-dev (<< ${ArrayFire_VERSION}), libarrayfire-opencl-dev (<< ${ArrayFire_VERSION})"
  REQUIRES "arrayfire-opencl${ArrayFire_VERSION_MAJOR} (>= ${ArrayFire_VERSION}), arrayfire-headers (>= ${ArrayFire_VERSION}), arrayfire-cmake (>= ${ArrayFire_VERSION})"
  OPTIONAL "cmake (>= 3.0)"
  )

af_deb_component(
  COMPONENT unified
  PACKAGE_NAME arrayfire-unified${ArrayFire_VERSION_MAJOR}
  PROVIDES "arrayfire-unified (= ${ArrayFire_VERSION}), arrayfire-unified${ArrayFire_VERSION_MAJOR} (= ${ArrayFire_VERSION}), libarrayfire-unified${ArrayFire_VERSION_MAJOR} (= ${ArrayFire_VERSION})"
  REPLACES "arrayfire-unified (<< ${ArrayFire_VERSION}), arrayfire-unified${ArrayFire_VERSION_MAJOR} (<< ${ArrayFire_VERSION}), libarrayfire-unified${ArrayFire_VERSION_MAJOR} (<< ${ArrayFire_VERSION})"
  REQUIRES "arrayfire-cpu (>= ${ArrayFire_VERSION}) | arrayfire-cuda (>= ${ArrayFire_VERSION}) | arrayfire-opencl (>= ${ArrayFire_VERSION})")

af_deb_component(
  COMPONENT unified-dev
  PACKAGE_NAME arrayfire-unified${ArrayFire_VERSION_MAJOR}-dev
  PROVIDES "arrayfire-unified-dev (= ${ArrayFire_VERSION}), arrayfire-unified${ArrayFire_VERSION_MAJOR}-dev (= ${ArrayFire_VERSION}), libarrayfire-unified-dev (= ${ArrayFire_VERSION})"
  REPLACES "arrayfire-unified-dev (<< ${ArrayFire_VERSION}), arrayfire-unified${ArrayFire_VERSION_MAJOR}-dev (<< ${ArrayFire_VERSION}), libarrayfire-unified-dev (<< ${ArrayFire_VERSION})"
  REQUIRES arrayfire-unified${ArrayFire_VERSION_MAJOR} arrayfire-cmake
  OPTIONAL "cmake (>= 3.0)"
  )

af_deb_component(
  COMPONENT documentation
  PACKAGE_NAME arrayfire-doc
  REPLACES "arrayfire-doc (<< ${ArrayFire_VERSION}), libarrayfire-doc (<< ${ArrayFire_VERSION})")
