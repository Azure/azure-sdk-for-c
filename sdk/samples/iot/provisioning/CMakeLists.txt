# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

if (TRANSPORT_PAHO)

cmake_minimum_required (VERSION 3.10)

project (az_iot_provisioning_samples LANGUAGES C)

set(CMAKE_C_STANDARD 99)

# Provisioning (Certificates) Sample
add_executable (paho_iot_provisioning_example 
  ${CMAKE_CURRENT_LIST_DIR}/src/paho_iot_provisioning_example.c
)

target_link_libraries(paho_iot_provisioning_example
    PRIVATE
      az::iot::samples::common
)

# Provisioning (SAS) Sample
add_executable (paho_iot_provisioning_sas_example
  ${CMAKE_CURRENT_LIST_DIR}/src/paho_iot_provisioning_sas_example.c
)

target_link_libraries(paho_iot_provisioning_sas_example
  PRIVATE
    az::iot::samples::common
)

# Workaround for linker warning LNK4098: defaultlib 'LIBCMTD' conflicts with use of other libs
if (MSVC)
    set_target_properties("paho_iot_provisioning_example"
        PROPERTIES LINK_FLAGS
        "/NODEFAULTLIB:libcmtd.lib"
    )
    set_target_properties("paho_iot_provisioning_sas_example"
        PROPERTIES LINK_FLAGS
        "/NODEFAULTLIB:libcmtd.lib"
    )
endif()

endif() # TRANSPORT_PAHO
