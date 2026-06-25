# Overlay port: libuuid
#
# Why this overlay exists:
#   The upstream vcpkg `libuuid` port downloads the source from SourceForge via
#   `vcpkg_from_sourceforge(...)`. 1ES build agents permanently block sourceforge.net
#   (every mirror returns `curl error code 7 (Couldn't connect to server)`), so any
#   Linux job that builds libuuid fails. libuuid is a transitive dependency of both
#   `paho-mqtt` and `curl`, which the root vcpkg.json manifest installs on every
#   Linux job. See Azure/azure-sdk-for-c#3464 (same class as the cmocka/gitlab block).
#
# What this overlay changes:
#   It sources the *same* libuuid release (1.0.3) from the MacPorts distfiles mirror
#   instead of SourceForge. The SHA512 below is unchanged from the upstream port and
#   was re-verified against
#   https://distfiles.macports.org/libuuid/libuuid-1.0.3.tar.gz (identical bytes).
#   Every other build step is identical to the upstream port.
#
# Scope / lifetime:
#   This is a SHORT-TERM unblock. The durable fix is vcpkg + Terrapin (an approved
#   download mirror / asset cache) configured at the pipeline level, which removes the
#   need for any blocked-origin download. Remove this overlay once that is in place.

set(LIBUUID_VERSION 1.0.3)

vcpkg_download_distfile(ARCHIVE
    URLS "https://distfiles.macports.org/libuuid/libuuid-${LIBUUID_VERSION}.tar.gz"
    FILENAME "libuuid-${LIBUUID_VERSION}.tar.gz"
    SHA512 77488caccc66503f6f2ded7bdfc4d3bc2c20b24a8dc95b2051633c695e99ec27876ffbafe38269b939826e1fdb06eea328f07b796c9e0aaca12331a787175507
)

vcpkg_extract_source_archive(SOURCE_PATH
    ARCHIVE "${ARCHIVE}"
)

file(COPY
    "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt"
    "${CMAKE_CURRENT_LIST_DIR}/config.linux.h"
    "${CMAKE_CURRENT_LIST_DIR}/unofficial-libuuid-config.cmake.in"
    DESTINATION "${SOURCE_PATH}"
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
)

vcpkg_cmake_install()

set(prefix "${CURRENT_INSTALLED_DIR}")
set(exec_prefix \$\{prefix\})
set(libdir \$\{exec_prefix\}/lib)
set(includedir \$\{prefix\}/include)
configure_file("${SOURCE_PATH}/uuid.pc.in" "${SOURCE_PATH}/uuid.pc" @ONLY)
if(NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "release")
    file(INSTALL "${SOURCE_PATH}/uuid.pc" DESTINATION "${CURRENT_PACKAGES_DIR}/lib/pkgconfig")
endif()
if(NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "debug")
    file(INSTALL "${SOURCE_PATH}/uuid.pc" DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib/pkgconfig")
endif()

vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/unofficial-libuuid PACKAGE_NAME unofficial-libuuid)
vcpkg_fixup_pkgconfig()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/COPYING")

vcpkg_copy_pdbs()
