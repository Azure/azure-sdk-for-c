# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

# Overwrite the default function to keep track of dependencies for a target
# Creates a property for each target called global_target_depends_<target_name> and
# appends depedencies to it. Calls the default function at the end.
function(target_link_libraries _target)
    set(_mode "PUBLIC")
    foreach(_arg IN LISTS ARGN)
        # Skip if scope label
        if (NOT _arg MATCHES "INTERFACE|PUBLIC|PRIVATE|LINK_PRIVATE|LINK_PUBLIC|LINK_INTERFACE_LIBRARIES")
          # Append the depedency to the target property
          set_property(GLOBAL APPEND PROPERTY global_target_depends_${_target} ${_arg})
        endif()
    endforeach()
    _target_link_libraries(${_target} ${ARGN})
endfunction()

# Function to recursively get the dependencies of a target
function(get_link_dependencies _target _listvar)
    set(_worklist ${${_listvar}})
    if (TARGET ${_target})
        list(APPEND _worklist ${_target})
        get_property(_dependencies GLOBAL PROPERTY global_target_depends_${_target})
        foreach(_dependency IN LISTS _dependencies)
            if (NOT _dependency IN_LIST _worklist)
                get_link_dependencies(${_dependency} _worklist)
            endif()
        endforeach()
        set(${_listvar} "${_worklist}" PARENT_SCOPE)
    endif()
endfunction()

function(install_target_dep_sources _target)
  #Recursively get the dependencies of the target
  get_link_dependencies(${_target} _deps)

  # Iterate over each dependency of the target
  foreach(_dep IN LISTS _deps)
    # Get the source files for the dependency
    get_target_property(_srcs ${_dep} SOURCES)

    # Get the full path to the source files
    get_target_property(_src_dir ${_dep} SOURCE_DIR)

    # Get the include directory of the dependency
    get_target_property(_inc_dir ${_dep} INTERFACE_INCLUDE_DIRECTORIES)

    # If the depedency has an inc, install it to the build directory
    if(_inc_dir)
      add_custom_command(
        TARGET ${_target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${_inc_dir}
                ${CMAKE_CURRENT_BINARY_DIR}/inc)
    endif()

    # For each source file of the depedency, install it to the build directory
    foreach(_src IN LISTS _srcs)
        add_custom_command(
        TARGET ${_target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${_src_dir}/${_src}
                ${CMAKE_CURRENT_BINARY_DIR}/${_src})
    endforeach()
  endforeach()
endfunction()
