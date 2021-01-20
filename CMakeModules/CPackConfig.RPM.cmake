##
# RPM package
##

include(CMakeParseArguments)

#TODO(umar): remove, should be set form command line
# set(CPACK_RPM_PACKAGE_RELEASE 1)

set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")
set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_RPM_PACKAGE_LICENSE "BSD")
set(CPACK_RPM_PACKAGE_GROUP "Development/Libraries")
set(CPACK_RPM_PACKAGE_URL "${SITE_URL}")
set(CPACK_RPM_PACKAGE_VENDOR "ArrayFire")
#set(CPACK_RPM_CHANGELOG_FILE "${ArrayFire_SOURCE_DIR}/docs/pages/release_notes.md")


macro(af_rpm_component)
  cmake_parse_arguments(RC "" "COMPONENT;PACKAGE_NAME;SUMMARY;DESCRIPTION" "REQUIRES;OPTIONAL;PROVIDES;CONFLICTS" ${ARGN})

  string(REPLACE ";" ", " REQ "${RC_REQUIRES}")
  string(REPLACE ";" ", " OPT "${RC_OPTIONAL}")
  string(TOUPPER ${RC_COMPONENT} COMPONENT_UPPER)

  if(RC_PACKAGE_NAME)
      set(CPACK_RPM_${COMPONENT_UPPER}_PACKAGE_NAME "${RC_PACKAGE_NAME}")
  endif()
  # NOTE: Does not work in CentOS 6
  set(CPACK_RPM_${COMPONENT_UPPER}_PACKAGE_SUGGESTS ${OPT})
  set(CPACK_RPM_${COMPONENT_UPPER}_PACKAGE_REQUIRES "${REQ}")
  set(CPACK_RPM_${COMPONENT_UPPER}_PACKAGE_PROVIDES ${RC_PROVIDES})
  set(CPACK_RPM_${COMPONENT_UPPER}_PACKAGE_CONFLICTS ${RC_CONFLICTS})
endmacro()

set(CPACK_RPM_MAIN_COMPONENT arrayfire)

af_rpm_component(
  COMPONENT arrayfire
  REQUIRES arrayfire-cpu-dev ${arrayfire_cuda_dev_name} arrayfire-opencl-dev arrayfire-unified-dev arrayfire-examples arrayfire-doc
  )

if(USE_CPU_MKL)
  set(cpu_runtime_package_name arrayfire-cpu-mkl)
  set(cpu_runtime_requirements "intel-mkl-core-rt-2020.0-166, intel-mkl-gnu-rt-2020.0-166")
  set(cpu_runtime_conflicts "arrayfire-cpu-openblas")
else()
  set(cpu_runtime_package_name arrayfire-cpu-openblas)
  set(cpu_runtime_requirements "fftw-libs, blas, lapack")
  set(cpu_runtime_conflicts "arrayfire-cpu-mkl")
endif()

af_rpm_component(
  COMPONENT cpu
  PACKAGE_NAME ${cpu_runtime_package_name}
  PROVIDES "arrayfire-cpu"
  CONFLICTS ${cpu_runtime_conflicts}
  REQUIRES ${cpu_runtime_requirements}
  OPTIONAL forge)

af_rpm_component(
  COMPONENT cpu-dev
  REQUIRES arrayfire-cpu arrayfire-headers arrayfire-cmake
  )

af_rpm_component(
  COMPONENT cuda
  PACKAGE_NAME arrayfire-cuda-cuda-${CUDA_VERSION_MAJOR}-${CUDA_VERSION_MINOR}
  OPTIONAL forge)

af_rpm_component(
  COMPONENT cuda-dev
  PACKAGE_NAME arrayfire-cuda-dev
  REQUIRES ${arrayfire_cuda_runtime_name} arrayfire-headers arrayfire-cmake)

af_rpm_component(
  COMPONENT opencl
  PACKAGE_NAME arrayfire-opencl
  OPTIONAL forge)

af_rpm_component(
  COMPONENT opencl-dev
  REQUIRES arrayfire-opencl arrayfire-headers arrayfire-cmake)

af_rpm_component(
  COMPONENT unified
  PACKAGE_NAME arrayfire-unified
  OPTIONAL forge)

af_rpm_component(
  COMPONENT unified-dev
  REQUIRES arrayfire-unified arrayfire-headers arrayfire-cmake
  OPTIONAL forge)

af_rpm_component(
  COMPONENT documentation
  PACKAGE_NAME arrayfire-doc)

# NOTE: These commands do not work well for forge. The
# version or the package name is incorrect when you perform
# the package creation this way. The CPACK_PACKAGE_VERSION
# does not extend to components. The forge rpm package will
# need to be created in the forge project.

#af_rpm_component(
#  COMPONENT forge-lib
#  PACKAGE_NAME forge-runtime
#  DESCRIPTION  )

#cpack_add_component(forge-lib
#    DESCRIPTION "Forge runtime libraries")
#
#set(CPACK_RPM_FORGE-LIB_PACKAGE_NAME "forge-runtime")
#get_target_property(Forge_VERSION forge VERSION)
##set(CPACK_RPM_FORGE-RUNTIME_FILE_NAME "forge-runtime-${Forge_VERSION}-%{release}%{?dist}.%{_arch}.rpm")
#set(CPACK_RPM_FORGE-LIB_FILE_NAME "forge-runtime-${Forge_VERSION}-%{release}%{?dist}.%{_arch}.rpm")

# Does not work in CentOS 6
#set(CPACK_RPM_${COMPONENT_UPPER}_PACKAGE_SUGGESTS ${OPT})
#set(CPACK_RPM_FORGE-LIB_SUMMARY "Forge runtime librariers")
#set(CPACK_RPM_FORGE-LIB_VERSION ${Forge_VERSION})
#set(CPACK_RPM_FORGE-LIB_PACKAGE_REQUIRES "${REQ}")
