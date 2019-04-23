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
  cmake_parse_arguments(RC "" "COMPONENT;NAME;SUMMARY;DESCRIPTION" "REQUIRES;OPTIONAL" ${ARGN})

  string(REPLACE ";" ", " REQ "${RC_REQUIRES}")
  string(REPLACE ";" ", " OPT "${RC_OPTIONAL}")
  string(TOUPPER ${RC_COMPONENT} COMPONENT_UPPER)

  if(RC_NAME)
      set(CPACK_RPM_${COMPONENT_UPPER}_PACKAGE_NAME "${RC_NAME}")
  endif()
  # NOTE: Does not work in CentOS 6
  set(CPACK_RPM_${COMPONENT_UPPER}_PACKAGE_SUGGESTS ${OPT})
  set(CPACK_RPM_${COMPONENT_UPPER}_PACKAGE_REQUIRES "${REQ}")
endmacro()

set(CPACK_RPM_MAIN_COMPONENT arrayfire)

af_rpm_component(
  COMPONENT arrayfire
  REQUIRES arrayfire-cpu-dev ${arrayfire_cuda_dev_name} arrayfire-opencl-dev arrayfire-unified-dev arrayfire-examples arrayfire-doc
  )

af_rpm_component(
  COMPONENT cpu
  NAME arrayfire-cpu
  OPTIONAL forge)

af_rpm_component(
  COMPONENT cpu-dev
  REQUIRES arrayfire-cpu arrayfire-headers arrayfire-cmake arrayfire-cpu-cmake
  )

af_rpm_component(
  COMPONENT cuda
  NAME ${arrayfire_cuda_runtime_name}
  OPTIONAL forge)

af_rpm_component(
  COMPONENT cuda-dev
  NAME ${arrayfire_cuda_dev_name}
  REQUIRES ${arrayfire_cuda_runtime_name} arrayfire-headers arrayfire-cmake arrayfire-cuda-cmake)

af_rpm_component(
  COMPONENT opencl
  NAME arrayfire-opencl
  OPTIONAL forge)

af_rpm_component(
  COMPONENT opencl-dev
  REQUIRES arrayfire-opencl arrayfire-headers arrayfire-cmake arrayfire-opencl-cmake)

af_rpm_component(
  COMPONENT unified
  NAME arrayfire-unified
  OPTIONAL forge)

af_rpm_component(
  COMPONENT unified-dev
  REQUIRES arrayfire-unified arrayfire-headers arrayfire-cmake arrayfire-unified-cmake
  OPTIONAL forge)

af_rpm_component(
  COMPONENT documentation
  NAME arrayfire-doc)

# NOTE: These commands do not work well for forge. The
# version or the package name is incorrect when you perform
# the package creation this way. The CPACK_PACKAGE_VERSION
# does not extend to components. The forge rpm package will
# need to be created in the forge project.

#af_rpm_component(
#  COMPONENT forge-lib
#  NAME forge-runtime
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
