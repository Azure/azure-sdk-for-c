# Overlay port: cmocka
#
# Why this overlay exists:
#   The upstream vcpkg `cmocka` port downloads the source from gitlab.com via
#   `vcpkg_from_gitlab(...)`. 1ES build agents permanently block gitlab.com, so the
#   download fails hard with `curl error code 7 (Couldn't connect to server)` and the
#   Linux unit-test job cannot build. See Azure/azure-sdk-for-c#3464.
#
# What this overlay changes:
#   It sources the *same* cmocka release (2.0.2) from the project's official file
#   server (https://cmocka.org/files) instead of gitlab.com. The SHA512 below was
#   computed from https://cmocka.org/files/2.0/cmocka-2.0.2.tar.xz. Every other build
#   step is identical to the upstream port.
#
# Scope / lifetime:
#   This is a SHORT-TERM unblock. cmocka.org is itself slated to be denied on build
#   agents, so the durable fix is vcpkg + Terrapin (an approved download mirror /
#   asset cache) configured at the pipeline level. Remove this overlay once Terrapin
#   (or an equivalent approved asset source) is in place. See #3464 for details.

vcpkg_download_distfile(ARCHIVE
    URLS "https://cmocka.org/files/2.0/cmocka-${VERSION}.tar.xz"
    FILENAME "cmocka-${VERSION}.tar.xz"
    SHA512 d02d65f0881f18f30b9e46c325acfa349261339daa2c1bf3a4e6360976f13b31588e997415197220f6def156f77d9864994d4e3cfd09c8f16a8594d0a4789a16
)

vcpkg_extract_source_archive(SOURCE_PATH
    ARCHIVE "${ARCHIVE}"
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DUNIT_TESTING=OFF
        -DWITH_EXAMPLES=OFF
        -DPICKY_DEVELOPER=OFF
)

vcpkg_cmake_install()

vcpkg_copy_pdbs()

vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/${PORT})
vcpkg_fixup_pkgconfig()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
