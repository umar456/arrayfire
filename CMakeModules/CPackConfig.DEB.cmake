
##
# Debian package
##
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEB_COMPONENT_INSTALL ON)
#set(CMAKE_INSTALL_RPATH /usr/lib;${ArrayFire_BUILD_DIR}/third_party/forge/lib)
#set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE http://www.arrayfire.com)

set(CPACK_DEBIAN_PACKAGE_NAME "arrayfire")

macro(af_deb_component)
  cmake_parse_arguments(RC "" "COMPONENT;NAME;SUMMARY;DESCRIPTION" "REQUIRES;OPTIONAL" ${ARGN})

  string(REPLACE ";" ", " REQ "${RC_REQUIRES}")
  string(REPLACE ";" ", " OPT "${RC_OPTIONAL}")
  string(TOUPPER ${RC_COMPONENT} COMPONENT_UPPER)

  if(RC_NAME)
    set(CPACK_DEBIAN_${COMPONENT_UPPER}_PACKAGE_NAME "${RC_NAME}")
  endif()

  set(CPACK_DEBIAN_${COMPONENT_UPPER}_PACKAGE_SUGGESTS ${OPT})
  set(CPACK_DEBIAN_${COMPONENT_UPPER}_PACKAGE_DEPENDS "${REQ}")
endmacro()

af_deb_component(
  COMPONENT arrayfire
  REQUIRES arrayfire-cpu-dev ${arrayfire_cuda_dev_name} arrayfire-opencl-dev arrayfire-unified-dev arrayfire-examples arrayfire-doc
  )

set(CPACK_DEBIAN_ARRAYFIRE_FILE_NAME "arrayfire-3.8.0_amd64.deb")

af_deb_component(
  COMPONENT cpu
  NAME arrayfire-cpu
  OPTIONAL forge)

af_deb_component(
  COMPONENT cpu-dev
  REQUIRES arrayfire-cpu arrayfire-headers arrayfire-cmake arrayfire-cpu-cmake
  )

af_deb_component(
  COMPONENT cuda
  NAME ${arrayfire_cuda_runtime_name}
  OPTIONAL forge)

af_deb_component(
  COMPONENT cuda-dev
  NAME ${arrayfire_cuda_dev_name}
  REQUIRES ${arrayfire_cuda_runtime_name} arrayfire-headers arrayfire-cmake arrayfire-cuda-cmake)

af_deb_component(
  COMPONENT opencl
  NAME arrayfire-opencl
  OPTIONAL forge)

af_deb_component(
  COMPONENT opencl-dev
  REQUIRES arrayfire-opencl arrayfire-headers arrayfire-cmake arrayfire-opencl-cmake)

af_deb_component(
  COMPONENT unified
  NAME arrayfire-unified
  OPTIONAL forge)

af_deb_component(
  COMPONENT unified-dev
  REQUIRES arrayfire-unified arrayfire-headers arrayfire-cmake arrayfire-unified-cmake
  OPTIONAL forge)

af_deb_component(
  COMPONENT documentation
  NAME arrayfire-doc)
