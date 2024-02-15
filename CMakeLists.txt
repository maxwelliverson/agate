cmake_minimum_required(VERSION 3.21)

if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(Catch2_ROOT $ENV{VCPKG_INSTALLED_PATH})
endif()
find_package(Catch2 3 CONFIG)

set(INSTALL_BASE_DIR "C:\\Users\\Maxwell")

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${INSTALL_BASE_DIR}" CACHE PATH "Base installation directory" FORCE)
endif()


function(print_value var_)
    # set(optionalArgs CACHE)
    set(singleValueArgs)
    set(multiValueArgs)
    cmake_parse_arguments(PRINT_VAR "CACHE" "${singleValueArgs}" "${multiValueArgs}" ${ARGN})
    if(PRINT_VAR_CACHE)
        message(STATUS "${var_}: '$CACHE{${var_}}'")
    else ()
        message(STATUS "${var_}: '${${var_}}'")
    endif()
endfunction()

include(getTargetArch.cmake)

# Dumb fuckin workaround for a bug in CMake where MSVC expects runtime flags to be populated even when targeting assembly files, which do not need them or populate the variables.
set(CMAKE_ASM_COMPILE_OPTIONS_MSVC_RUNTIME_LIBRARY_MultiThreaded         "")
set(CMAKE_ASM_COMPILE_OPTIONS_MSVC_RUNTIME_LIBRARY_MultiThreadedDLL      "")
set(CMAKE_ASM_COMPILE_OPTIONS_MSVC_RUNTIME_LIBRARY_MultiThreadedDebug    "")
set(CMAKE_ASM_COMPILE_OPTIONS_MSVC_RUNTIME_LIBRARY_MultiThreadedDebugDLL "")

set(AGATE_VERSION_MAJOR 0)
set(AGATE_VERSION_MINOR 1)
set(AGATE_VERSION_PATCH 0)

set(AGATE_VERSION_STRING "${AGATE_VERSION_MAJOR}.${AGATE_VERSION_MINOR}.${AGATE_VERSION_PATCH}")



project(agate LANGUAGES CXX ASM_MASM VERSION ${AGATE_VERSION_STRING})

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 23)


if (DEFINED AGATE_RELWITHDEBINFO_BUILD)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        unset(CMAKE_CXX_FLAGS_DEBUG)
        set(CMAKE_CXX_FLAGS_DEBUG "/Zi /O2 /Ob2 /DNDEBUG")
        # add_compile_options(/O2)
    else()
        add_compile_options(-g)
    endif()
endif()




if (NOT DEFINED CACHE{AGATE_TARGET_ARCH})
    target_architecture(AGATE_TARGET_ARCH_TMP)
    message(STATUS "AGATE_TARGET_ARCH=${AGATE_TARGET_ARCH_TMP}")
    set(AGATE_TARGET_ARCH ${AGATE_TARGET_ARCH_TMP} CACHE INTERNAL "Target Architecture" FORCE)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        if (AGATE_TARGET_ARCH_TMP STREQUAL "x86")
            set(AGATE_ASM_FILE_PREFIX "x86_nt_masm" CACHE INTERNAL "" FORCE)
            set(AGATE_ASM_LANG ASM_MASM CACHE INTERNAL "" FORCE)
        elseif(AGATE_TARGET_ARCH_TMP STREQUAL "x64")
            set(AGATE_ASM_FILE_PREFIX "x64_nt_masm" CACHE INTERNAL "" FORCE)
            set(AGATE_ASM_LANG ASM_MASM CACHE INTERNAL "" FORCE)
        elseif(AGATE_TARGET_ARCH_TMP STREQUAL "arm")
            set(AGATE_ASM_FILE_PREFIX "arm_nt_masm" CACHE INTERNAL "" FORCE)
            set(AGATE_ASM_LANG ASM_MARMASM CACHE INTERNAL "" FORCE)
        elseif(AGATE_TARGET_ARCH_TMP STREQUAL "arm64")
            set(AGATE_ASM_FILE_PREFIX "arm64_nt_masm" CACHE INTERNAL "" FORCE)
            set(AGATE_ASM_LANG ASM_MARMASM CACHE INTERNAL "" FORCE)
        else()
            message(SEND_ERROR "Invalid target architecture for Windows: '${CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE}'   (expected 'x86', 'x64', or 'arm64')")
            message(FATAL_ERROR "Could not determine target architecture")
        endif()
        set(AGATE_ASM_FILE_EXTENSION "asm" CACHE INTERNAL "" FORCE)
    elseif()
        message(FATAL_ERROR "Only Windows MSVC is supported for the time being (should be available on linux and possible OSX)")
    endif()
endif()

print_value(AGATE_TARGET_ARCH CACHE)
print_value(AGATE_ASM_FILE_PREFIX CACHE)
print_value(AGATE_ASM_LANG CACHE)
print_value(AGATE_ASM_FILE_EXTENSION CACHE)

# print_value(CMAKE_CXX_FLAGS)
# print_value(CMAKE_CXX_FLAGS_DEBUG)
# print_value(CMAKE_CXX_FLAGS_RELEASE)






add_library(asan INTERFACE)
if (CMAKE_ASAN_ENABLED)
    set(ASAN_LIBRARY_DIRECTORY "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/Llvm/x64/lib/clang/12.0.0/lib/windows")
    target_compile_options(asan INTERFACE -fsanitize=address /Od)
    target_link_directories(asan INTERFACE "${ASAN_LIBRARY_DIRECTORY}")
endif()

function(add_agate_library target_)
    set(optionalArgs PRIVATE EXPORT)
    set(singleValueArgs)
    set(multiValueArgs SOURCES DEPENDENCIES DEFINES)
    cmake_parse_arguments(AGATE_LIB "${optionalArgs}" "${singleValueArgs}" "${multiValueArgs}" ${ARGN})
    list(TRANSFORM AGATE_LIB_SOURCES PREPEND src/${target_}/)
    if(AGATE_LIB_PRIVATE)
        add_library(agate.${target_} OBJECT ${AGATE_LIB_SOURCES})
        target_include_directories(agate.${target_} PRIVATE include src)
        if(DEFINED AGATE_LIB_DEPENDENCIES)
            target_link_libraries(agate.${target_} PRIVATE ${AGATE_LIB_DEPENDENCIES})
        endif()
    else()
        list(APPEND AGATE_LIBRARIES ${target_})
        set(AGATE_LIBRARIES ${AGATE_LIBRARIES} PARENT_SCOPE)
        set(AGATE_LIB_SHARED_NAME ${target_}_shared)
        set(AGATE_LIB_STATIC_NAME ${target_}_static)

        add_library(${AGATE_LIB_SHARED_NAME} SHARED ${AGATE_LIB_SOURCES})
        add_library(${AGATE_LIB_STATIC_NAME} STATIC ${AGATE_LIB_SOURCES})

        if(DEFINED AGATE_LIB_DEPENDENCIES)
            foreach(dep IN LISTS AGATE_LIB_DEPENDENCIES)
                list(FIND AGATE_LIBRARIES ${dep} IS_AGATE_LIB)
                if(IS_AGATE_LIB GREATER -1)
                    list(APPEND AGATE_LIB_STATIC_DEPENDENCIES ${dep}_static)
                    list(APPEND AGATE_LIB_SHARED_DEPENDENCIES ${dep}_shared)
                else()
                    list(APPEND AGATE_LIB_STATIC_DEPENDENCIES $<TARGET_OBJECTS:agate.${dep}>)
                    list(APPEND AGATE_LIB_SHARED_DEPENDENCIES $<TARGET_OBJECTS:agate.${dep}>)
                endif()
            endforeach()

            target_link_libraries(${AGATE_LIB_STATIC_NAME} PRIVATE ${AGATE_LIB_STATIC_DEPENDENCIES})
            target_link_libraries(${AGATE_LIB_SHARED_NAME} PRIVATE ${AGATE_LIB_SHARED_DEPENDENCIES})
        endif()

        set_target_properties(${AGATE_LIB_SHARED_NAME}
                PROPERTIES
                OUTPUT_NAME ${target_})
        target_include_directories(${AGATE_LIB_SHARED_NAME} PRIVATE include src)
        target_include_directories(${AGATE_LIB_STATIC_NAME} PRIVATE include src)
        target_compile_definitions(${AGATE_LIB_SHARED_NAME} PUBLIC AGT_SHARED_LIB PRIVATE AGT_SHARED_LIB_EXPORT)

    endif()
endfunction()


function(add_agate_module target_)
    set(optionalArgs EXPORT)
    set(singleValueArgs ASM_BASEDIR)
    set(multiValueArgs SOURCES DEPENDENCIES DEFINES ASM_SOURCES)
    cmake_parse_arguments(AGATE_MODULE "${optionalArgs}" "${singleValueArgs}" "${multiValueArgs}" ${ARGN})
    list(TRANSFORM AGATE_MODULE_SOURCES PREPEND src/modules/${target_}/)

    set(ASM_SOURCE_OBJECTS "")

    if (DEFINED AGATE_MODULE_ASM_SOURCES)
        set(ASM_BASEDIR "src/modules/${target_}")
        message(STATUS "ASM_BASEDIR before conditional=${ASM_BASEDIR}")
        if (DEFINED AGATE_MODULE_ASM_BASEDIR)
            message(STATUS "AGATE_MODULE_ASM_BASEDIR is defined (= '${AGATE_MODULE_ASM_BASEDIR}' )")
            if (AGATE_MODULE_ASM_BASEDIR MATCHES "^[/\\]")
                message(FATAL_ERROR "Value of ASM_BASEDIR argument to add_agate_module must not begin with a slash.")
            endif()
            message(STATUS "ASM_BASEDIR before append=${ASM_BASEDIR}")
            string(APPEND ASM_BASEDIR "/${AGATE_MODULE_ASM_BASEDIR}")
            message(STATUS "ASM_BASEDIR after append=${ASM_BASEDIR}")
            string(REGEX REPLACE "[/\\]$" "" ASM_BASEDIR ${ASM_BASEDIR}) # strips the end of the string of any slashes
            message(STATUS "ASM_BASEDIR after regex replace=${ASM_BASEDIR}")
        endif ()

        foreach(asm_source ${AGATE_MODULE_ASM_SOURCES})
            set(ASM_TARGET_NAME "agate.${target_}.asm.${asm_source}")
            set(ASM_SOURCE_FILE "${ASM_BASEDIR}/$CACHE{AGATE_ASM_FILE_PREFIX}-${asm_source}.$CACHE{AGATE_ASM_FILE_EXTENSION}")
            add_library(${ASM_TARGET_NAME} OBJECT ${ASM_SOURCE_FILE})
            set(ASM_SOURCE_OBJECTS "${ASM_SOURCE_OBJECTS};$<TARGET_OBJECTS:${ASM_TARGET_NAME}>")
        endforeach ()
    endif()


    set(AGATE_MODULE_EXPORTS_FILE src/modules/${target_}/exports.cpp)

    add_library(agate.${target_}        SHARED ${AGATE_MODULE_EXPORTS_FILE} ${AGATE_MODULE_SOURCES} ${ASM_SOURCE_OBJECTS})

    string(TOUPPER ${target_} MODULE_NAME_DEF)
    string(PREPEND MODULE_NAME_DEF AGT_BUILDING_)
    string(APPEND MODULE_NAME_DEF _MODULE)

    target_compile_definitions(agate.${target_}        PRIVATE
            AGT_BUILD_SHARED
            AGT_BUILDING_MODULE
            ${MODULE_NAME_DEF}
            AGT_MODULE_VERSION_MAJOR=${AGATE_VERSION_MAJOR}
            AGT_MODULE_VERSION_MINOR=${AGATE_VERSION_MINOR}
            AGT_MODULE_VERSION_PATCH=${AGATE_VERSION_PATCH})

    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(agate.${target_} PRIVATE /Zc:preprocessor)
    endif()


    target_include_directories(agate.${target_} PRIVATE include src src/modules)

    if(DEFINED AGATE_LIB_DEPENDENCIES)
        target_link_libraries(agate.${target_} PRIVATE agate-support ${AGATE_LIB_DEPENDENCIES})
    else()
        target_link_libraries(agate.${target_}        PRIVATE agate-support)
    endif()

    set_target_properties(agate.${target_} PROPERTIES OUTPUT_NAME agate-${target_})

    list(APPEND AGATE_MODULES agate.${target_})
    set(AGATE_MODULES ${AGATE_MODULES} PARENT_SCOPE)
endfunction()


function(add_agate_executable target_)
    set(optionalArgs TEST EXAMPLE)
    set(singleValueArgs LINK)
    set(multiValueArgs SOURCES DEPENDENCIES OPTIONS)
    cmake_parse_arguments(AGATE_EXE "${optionalArgs}" "${singleValueArgs}" "${multiValueArgs}" ${ARGN})
    if(AGATE_EXE_TEST)
        if(target_ MATCHES "catch")
            set(AGATE_EXE_TARGET ${target_})
            set(AGATE_EXE_SOURCE_PREFIX "test")
        else()
            set(AGATE_EXE_TARGET test.${target_})
            set(AGATE_EXE_SOURCE_PREFIX "${AGATE_EXE_TARGET}")
        endif()
    elseif(AGATE_EXE_EXAMPLE)
        set(AGATE_EXE_TARGET example.${target_})
        set(AGATE_EXE_SOURCE_PREFIX "${AGATE_EXE_TARGET}")
    else()
        set(AGATE_EXE_TARGET ${target_})
        set(AGATE_EXE_SOURCE_PREFIX "src.${AGATE_EXE_TARGET}")
    endif()
    # message("AGATE_EXE_SOURCE_PREFIX: ${AGATE_EXE_SOURCE_PREFIX}")
    string(REGEX REPLACE "\\." "/" AGATE_EXE_SOURCE_PREFIX ${AGATE_EXE_SOURCE_PREFIX})
    list(TRANSFORM AGATE_EXE_SOURCES PREPEND "${AGATE_EXE_SOURCE_PREFIX}/")
    add_executable(${AGATE_EXE_TARGET} ${AGATE_EXE_SOURCES})
    if (AGATE_EXE_TEST)
        target_include_directories(${AGATE_EXE_TARGET} PUBLIC include src test)
    else()
        target_include_directories(${AGATE_EXE_TARGET} PUBLIC include)
    endif()

    if(NOT DEFINED AGATE_EXE_LINK)
        set(AGATE_EXE_LINK static)
    endif()
    if(DEFINED AGATE_EXE_DEPENDENCIES)
        foreach(dep IN LISTS AGATE_EXE_DEPENDENCIES)
            list(FIND AGATE_LIBRARIES ${dep} IS_AGATE_LIB)
            if(IS_AGATE_LIB GREATER -1)
                list(APPEND AGATE_EXE_REAL_DEPENDENCIES ${dep}_${AGATE_EXE_LINK})
            else()
                list(APPEND AGATE_EXE_REAL_DEPENDENCIES ${dep})
            endif()
        endforeach()

        target_link_libraries(${AGATE_EXE_TARGET} PRIVATE ${AGATE_EXE_REAL_DEPENDENCIES})
    endif()
    if(AGATE_EXE_TEST)
        set_target_properties(${AGATE_EXE_TARGET} PROPERTIES EXCLUDE_FROM_ALL 1)
    endif()
    if(DEFINED AGATE_EXE_OPTIONS)
        target_compile_options(${AGATE_EXE_TARGET} PRIVATE ${AGATE_EXE_OPTIONS})
    endif()
endfunction()







#set(CMAKE_ASM_MASM_COMPILER )

#set(CMAKE_ASM-)




add_library(agate-support
            SHARED
            # src/agate/atomic.cpp
            src/agate/codex.cpp
            src/agate/dictionary.cpp
            src/agate/dynamic_code.cpp
            src/agate/processor.cpp
            src/agate/thread.cpp
            src/agate/vector.cpp)
target_include_directories(agate-support PRIVATE include src)


add_agate_module(
        core
            PRIVATE
            SOURCES
                async.cpp
                ctx.cpp
                error.cpp
                fiber.cpp
                instance.cpp
                # object_allocator.cpp
                random.cpp
                segment.cpp
                shared.cpp
                rcpool.cpp
                rcmem.cpp
                rcweak.cpp
            ASM_BASEDIR
                asm
            ASM_SOURCES
                afiber_fork
                afiber_init
                afiber_jump
                afiber_loop
                afiber_switch
)


add_agate_module(
        agents
            PRIVATE
            SOURCES
                agents.cpp
                dispatch.cpp
                executor.cpp
                send.cpp
)

add_agate_module(
        channels
            PRIVATE
            SOURCES
                channels.cpp
                message_queue.cpp
                broadcast_queue.cpp
                sender.cpp
                receiver.cpp
                message.cpp
                message_pool.cpp
                ring_queue.cpp
)

add_agate_module(
        pool
            PRIVATE
            SOURCES
                st_pool.cpp
                mt_pool.cpp
                st_rcpool.cpp
                mt_rcpool.cpp
                st_rcmem.cpp
                mt_rcmem.cpp
                st_rcweak.cpp
                mt_rcweak.cpp
)

add_agate_module(
        log
            PRIVATE
            SOURCES
)

add_agate_module(
        network
            PRIVATE
            SOURCES
                st_net.cpp
                mt_net.cpp
)



#[[add_agate_library(
        agate
            SOURCES
                agate.cpp
            DEPENDENCIES
                agents
                async
                channels
                core
                support
                threads
)]]


add_library(agate STATIC src/agate.cpp src/config.cpp)
target_include_directories(agate PRIVATE src include src/modules)
target_link_libraries(agate PUBLIC agate-support)
target_compile_definitions(agate PRIVATE AGT_BUILDING_CORE_LIBRARY)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_link_libraries(agate-support PUBLIC Synchronization.lib)
    target_compile_options(agate-support PRIVATE /Zc:preprocessor)
    target_compile_options(agate PRIVATE /Zc:preprocessor)
endif()


#[[add_library(agate-static STATIC src/agate.cpp src/loader.cpp)
target_include_directories(agate-static PRIVATE src include)
target_link_libraries(agate-static PRIVATE
        agate-support
        $<TARGET_OBJECTS:agate-static.core>
        $<TARGET_OBJECTS:agate-static.agents>
        $<TARGET_OBJECTS:agate-static.async>
        $<TARGET_OBJECTS:agate-static.channels>
        $<TARGET_OBJECTS:agate-static.threads>
        $<TARGET_OBJECTS:agate-static.log>
        $<TARGET_OBJECTS:agate-static.network>
        $<TARGET_OBJECTS:agate-static.pool>)
target_compile_definitions(agate-static PRIVATE AGT_BUILD_STATIC AGT_BUILDING_CORE_LIBRARY)]]








add_agate_executable(
        command_buffers
            TEST
            SOURCES
                main.cpp
            DEPENDENCIES
                agate
)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_agate_executable(
            coro
                TEST
                SOURCES
                    main.cpp
                DEPENDENCIES
                    agate
                OPTIONS
                    /await:strict
    )
endif()


add_agate_executable(
        ring_benchmark
            TEST
            SOURCES
                main.cpp
            DEPENDENCIES
                agate
)



add_agate_executable(
        alloc_benchmarks
            TEST
            SOURCES
                main.cpp
)

add_agate_executable(
        cpuinfo
            TEST
            SOURCES
                main.cpp
                ../../src/agate/processor.cpp
)

add_agate_executable(
        fibers
            EXAMPLE
            SOURCES
                main.cpp
)




if (Catch2_FOUND)


    add_library(config-test-first SHARED test/catch/first_dll.cpp)
    add_library(config-test-second SHARED test/catch/second_dll.cpp)
    add_library(config-test-third SHARED test/catch/third_dll.cpp)
    target_include_directories(config-test-first PRIVATE include)
    target_include_directories(config-test-second PRIVATE include)
    target_include_directories(config-test-third PRIVATE include)
    target_link_libraries(config-test-first PRIVATE agate config-test-third)
    target_link_libraries(config-test-second PRIVATE agate)
    target_link_libraries(config-test-third PRIVATE agate)
    target_compile_definitions(config-test-first PRIVATE AGATE_COMPILING_FIRST_DLL)
    target_compile_definitions(config-test-second PRIVATE AGATE_COMPILING_SECOND_DLL)
    target_compile_definitions(config-test-third PRIVATE AGATE_COMPILING_THIRD_DLL)

    add_executable(tests test/catch/config.cpp test/catch/fibers.cpp)
    target_link_libraries(tests PRIVATE agate config-test-first config-test-second config-test-third Catch2::Catch2WithMain)
    add_dependencies(tests agate.core agate.log)

    include(CTest)
    include(Catch)
    catch_discover_tests(tests)
endif()


add_executable(scratch test/scratch.cpp)



add_library(DUMMY src/dummy.cpp)
target_include_directories(DUMMY PRIVATE include src)

add_library(DUMMY_MODULE src/dummy_module.cpp)
target_include_directories(DUMMY_MODULE PRIVATE include src)





target_sources(agate PUBLIC FILE_SET HEADERS BASE_DIRS include
        FILES
        include/agate.h
        include/agate_module.h
        include/agate/agents.h
        include/agate/async.h
        include/agate/channels.h
        include/agate/core.h
        include/agate/exec.h
        include/agate/fiber.h
        include/agate/init.h
        include/agate/log.h
        include/agate/naming.h
        include/agate/network.h
        include/agate/pool.h
        include/agate/shmem.h
        include/agate/signal.h
        include/agate/task.h
        include/agate/thread.h
        include/agate/timer.h
)



include(GNUInstallDirs)

set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR}
        CACHE PATH "Location of header files" )
set(SYSCONFIG_INSTALL_DIR ${CMAKE_INSTALL_SYSCONFDIR}/agate
        CACHE PATH "Location of configuration files" )
set(LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}
        CACHE PATH "Location of library files")
set(BIN_INSTALL_DIR ${CMAKE_INSTALL_BINDIR}
        CACHE PATH "Location of binary files")


install(TARGETS agate agate-support
        EXPORT agateTargets
        FILE_SET HEADERS
        LIBRARY DESTINATION $CACHE{BIN_INSTALL_DIR}       # agate modules
        ARCHIVE DESTINATION $CACHE{LIB_INSTALL_DIR}       # agate.lib (the loader)
        INCLUDES DESTINATION $CACHE{INCLUDE_INSTALL_DIR}) # header files

install(TARGETS ${AGATE_MODULES}
        RUNTIME DESTINATION $CACHE{BIN_INSTALL_DIR}
        LIBRARY DESTINATION $CACHE{LIB_INSTALL_DIR}
)

install(EXPORT agateTargets
        FILE agateTargets.cmake
        NAMESPACE agate::
        DESTINATION $CACHE{LIB_INSTALL_DIR}/cmake/agate)

include(CMakePackageConfigHelpers)
configure_package_config_file(agateConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/agateConfig.cmake
        INSTALL_DESTINATION $CACHE{LIB_INSTALL_DIR}/cmake/agate
        PATH_VARS INCLUDE_INSTALL_DIR SYSCONFIG_INSTALL_DIR)
write_basic_package_version_file("agateConfigVersion.cmake" COMPATIBILITY SameMajorVersion)

# install(DIRECTORY include/dt TYPE INCLUDE)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/agateConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/agateConfigVersion.cmake" DESTINATION $CACHE{LIB_INSTALL_DIR}/cmake/agate)