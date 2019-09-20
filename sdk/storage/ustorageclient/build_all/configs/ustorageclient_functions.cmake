#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#Macro to set C and Cpp standards for a given target
macro(add_standards)
    set(CMAKE_C_STANDARD                99)
    set(CMAKE_C_STANDARD_REQUIRED       OFF)
    set(CMAKE_C_EXTENSIONS              OFF)

    set(CMAKE_CXX_STANDARD              11)
    set(CMAKE_CXX_STANDARD_REQUIRED     OFF)
    set(CMAKE_CXX_EXTENSIONS            OFF)
endmacro()

#Add windows unittest dll
function(azstorage_c_windows_unittests_add_dll what_is_building folder)

    link_directories(${what_is_building}_dll $ENV{VCInstallDir}UnitTest/lib)

    #Add static library target
    add_library(${what_is_building}_testsonly_lib STATIC
            ${${what_is_building}_test_files}
    )

    #Set the output folder for the target
    set_target_properties(${what_is_building}_testsonly_lib
               PROPERTIES
               FOLDER ${folder} )

    target_include_directories(${what_is_building}_testsonly_lib PUBLIC ${sharedutil_include_directories} $ENV{VCInstallDir}UnitTest/include)
    target_compile_definitions(${what_is_building}_testsonly_lib PUBLIC CPP_UNITTEST)
    target_compile_options(${what_is_building}_testsonly_lib PUBLIC /TP /EHsc)

    #Add shared/dynamic libary target
    add_library(${what_is_building}_dll SHARED
        ${${what_is_building}_cpp_files}
        ${${what_is_building}_c_files}
    )

    #Set the output folder for the target
    set_target_properties(${what_is_building}_dll
               PROPERTIES
               FOLDER ${folder})

    #Mark C source files as "C source code"
    set_source_files_properties(${${what_is_building}_c_files}
               PROPERTIES
               COMPILE_FLAGS /TC)

    #Mark Cpp source files as "C++ source code"
    set_source_files_properties(${${what_is_building}_cpp_files}
               PROPERTIES
               COMPILE_FLAGS /TP)

    #Link necessary libs to dll library
    target_link_libraries(${what_is_building}_dll
                #add azure_ulib_c back later
                PRIVATE umock_c ctest testrunnerswitcher azure_iot_ustorageclient ${what_is_building}_testsonly_lib
    )

endfunction()

#Add windows unit test exe
function(azstorage_c_windows_unittests_add_exe what_is_building folder)

    #Define target
    add_executable(${what_is_building}_exe
        ${${what_is_building}_test_files}
        ${${what_is_building}_cpp_files}
        ${${what_is_building}_c_files}
        ${CMAKE_CURRENT_LIST_DIR}/main.c
    )

    #Set the output folder for the target
    set_target_properties(${what_is_building}_exe
               PROPERTIES
               FOLDER ${folder})

    #Define executable's compile definitions, linked libraries, and include dirs
    target_compile_definitions(${what_is_building}_exe PUBLIC USE_CTEST)
    target_include_directories(${what_is_building}_exe PUBLIC ${sharedutil_include_directories})
    target_link_libraries(${what_is_building}_exe
                #add azure_ulib_c back later
                PRIVATE umock_c ctest testrunnerswitcher azure_iot_ustorageclient
    )

    #Add test to ctest list
    add_test(NAME ${what_is_building} COMMAND ${what_is_building}_exe)

endfunction()

#Build Linux unit tests with valgrind if enabled
function(azstorage_c_linux_unittests_add_exe what_is_building folder)

    #Define target
    add_executable(${what_is_building}_exe
        ${${what_is_building}_test_files}
        ${${what_is_building}_cpp_files}
        ${${what_is_building}_c_files}
        ${CMAKE_CURRENT_LIST_DIR}/main.c
    )

    #Set the output folder for the target
    set_target_properties(${what_is_building}_exe
               PROPERTIES
               FOLDER ${folder})

    #Define executable's compile definitions, linked libraries, and include dirs
    target_compile_definitions(${what_is_building}_exe PUBLIC USE_CTEST)
    target_include_directories(${what_is_building}_exe PUBLIC ${sharedutil_include_directories})
    target_link_libraries(${what_is_building}_exe
                #add azure_ulib_c back later
                PRIVATE umock_c ctest testrunnerswitcher azure_iot_ustorageclient
    )

    #Add test to ctest list
    add_test(NAME ${what_is_building} COMMAND $<TARGET_FILE:${what_is_building}_exe>)

    #Setup valgrind if applicable and add separate test for valgrind/helgrind
    if(${run_valgrind})
        find_program(VALGRIND_FOUND NAMES valgrind)
        if(${VALGRIND_FOUND} STREQUAL VALGRIND_FOUND-NOTFOUND)
            message(WARNING "run_valgrind was TRUE, but valgrind was not found - there will be no tests run under valgrind")
        else()
            add_test(NAME ${what_is_building}_valgrind COMMAND valgrind                 --gen-suppressions=all --num-callers=100 --error-exitcode=1 --leak-check=full --track-origins=yes $<TARGET_FILE:${what_is_building}_exe>)
            add_test(NAME ${what_is_building}_helgrind COMMAND valgrind --tool=helgrind --gen-suppressions=all --num-callers=100 --error-exitcode=1 $<TARGET_FILE:${what_is_building}_exe>)
            add_test(NAME ${what_is_building}_drd      COMMAND valgrind --tool=drd      --gen-suppressions=all --num-callers=100 --error-exitcode=1 $<TARGET_FILE:${what_is_building}_exe>)
        endif()
    endif()

endfunction()

function(azstorage_build_c_test_target what_is_building folder)
    #Include repo directories
    include_directories(${PROJECT_SOURCE_DIR}/inc)
    include_directories(${PROJECT_SOURCE_DIR}/config)

    #Set shared utility include directories
    #add ${AZURE_ULIB_C_INC_FOLDER} back later
    set(ustorageclient_test_framework_includes ${sharedutil_include_directories} ${MACRO_UTILS_INC_FOLDER} ${UMOCK_C_INC_FOLDER} ${TESTRUNNERSWITCHER_INC_FOLDER} ${CTEST_INC_FOLDER})
    include_directories(${ustorageclient_test_framework_includes})

    #Create Windows/Linux test executables
    if(WIN32)
        if (${use_cppunittest})
            azstorage_c_windows_unittests_add_dll(${what_is_building} ${folder} ${ARGN})
        endif()
        azstorage_c_windows_unittests_add_exe(${what_is_building} ${folder} ${ARGN})
    else()
        azstorage_c_linux_unittests_add_exe(${what_is_building} ${folder} ${ARGN})
    endif()

endfunction()

#Build sample
function(azstorage_build_c_sample_target what_is_building folder)
    #Define target
    add_executable(${what_is_building}_exe
        ${${what_is_building}_c_files}
    )

    #Set the output folder for the target
    set_target_properties(${what_is_building}_exe
               PROPERTIES
               FOLDER ${folder})

    #Define executable's compile definitions, linked libraries, and include dirs
    target_include_directories(${what_is_building}_exe PUBLIC
                ${MACRO_UTILS_INC_FOLDER}
                ${UMOCK_C_INC_FOLDER}
                ${PROJECT_SOURCE_DIR}/inc
                ${PROJECT_SOURCE_DIR}/config
                ${${what_is_building}_h_dir}
    )
    target_link_libraries(${what_is_building}_exe
                PRIVATE azure_iot_ustorageclient
                #PRIVATE azure_ulib_c
                PRIVATE aziotsharedutil
    )


endfunction()

#unit tests
function(add_unittest_directory test_directory)
    if (${run_azstorage_unit_tests})
        add_subdirectory(${test_directory})
    endif()
endfunction()