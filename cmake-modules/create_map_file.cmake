# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT
#
# Instructs linker to generate map files and optimize build for minimal size
# Requires CMake version >= 3.13 to use add_link_options

function(create_map_file MAP_FILE_NAME)
    if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.13")
        if(MSVC)
            add_link_options(/MAP)
        elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
            add_link_options(-Wl,-map,${MAP_FILE_NAME})
            add_link_options(-Os)
        else()
            add_link_options(-Xlinker -Map=${MAP_FILE_NAME})
            add_link_options(-Os)
        endif()
    else()
        message("Skipping map file generation because CMake version does not support add_link_options")
    endif()
endfunction()
