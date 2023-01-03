cmake_minimum_required(VERSION 3.21)
project(agate)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 23)


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
    set(singleValueArgs)
    set(multiValueArgs SOURCES DEPENDENCIES DEFINES)
    cmake_parse_arguments(AGATE_MODULE "${optionalArgs}" "${singleValueArgs}" "${multiValueArgs}" ${ARGN})
    list(TRANSFORM AGATE_MODULE_SOURCES PREPEND src/${target_}/)

    add_library(agate.${target_}        SHARED src/agate/module.cpp ${AGATE_MODULE_SOURCES})
    add_library(agate-static.${target_} OBJECT ${AGATE_MODULE_SOURCES})

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
    target_compile_definitions(agate-static.${target_} PRIVATE
            AGT_BUILD_STATIC
            AGT_BUILDING_MODULE
            ${MODULE_NAME_DEF})


    target_include_directories(agate.${target_} PRIVATE include src)
    target_include_directories(agate-static.${target_} PRIVATE include src)

    if(DEFINED AGATE_LIB_DEPENDENCIES)
        target_link_libraries(agate.${target_} PRIVATE agate-support ${AGATE_LIB_DEPENDENCIES})
        target_link_libraries(agate-static.${target_} PRIVATE agate-support ${AGATE_LIB_DEPENDENCIES})
    else()
        target_link_libraries(agate.${target_}        PRIVATE agate-support)
        target_link_libraries(agate-static.${target_} PRIVATE agate-support)
    endif()

    set_target_properties(agate.${target_} PROPERTIES OUTPUT_NAME agate-${target_})
endfunction()


function(add_agate_executable target_)
    set(optionalArgs TEST SAMPLE)
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
    elseif(AGATE_EXE_SAMPLE)
        set(AGATE_EXE_TARGET sample.${target_})
        set(AGATE_EXE_SOURCE_PREFIX "${AGATE_EXE_TARGET}")
    else()
        set(AGATE_EXE_TARGET ${target_})
        set(AGATE_EXE_SOURCE_PREFIX "src.${AGATE_EXE_TARGET}")
    endif()
    # message("AGATE_EXE_SOURCE_PREFIX: ${AGATE_EXE_SOURCE_PREFIX}")
    string(REGEX REPLACE "\\." "/" AGATE_EXE_SOURCE_PREFIX ${AGATE_EXE_SOURCE_PREFIX})
    list(TRANSFORM AGATE_EXE_SOURCES PREPEND "${AGATE_EXE_SOURCE_PREFIX}/")
    add_executable(${AGATE_EXE_TARGET} ${AGATE_EXE_SOURCES})
    target_include_directories(${AGATE_EXE_TARGET} PUBLIC include src)
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


set(AGATE_VERSION_MAJOR 0)
set(AGATE_VERSION_MINOR 1)
set(AGATE_VERSION_PATCH 0)




add_library(agate-support
        src/agate/atomic.cpp
        src/agate/dictionary.cpp
        src/agate/vector.cpp)


add_agate_module(
        agents
            PRIVATE
            SOURCES
                agents.cpp
                dispatch.cpp
                executor.cpp
                state.cpp
                send.cpp
)

add_agate_module(
        async
            PRIVATE
            SOURCES
                async.cpp
                signal.cpp
                wait.cpp
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
)


add_agate_module(
        core
            PRIVATE
            SOURCES
                context.cpp
                error.cpp
                init.cpp
                object_allocator.cpp
                random.cpp
                segment.cpp
                shared.cpp
                thread_state.cpp
                rcpool.cpp
                rcmem.cpp
                rcweak.cpp
)


add_agate_module(
        threads
            PRIVATE
            SOURCES
                thread.cpp
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


add_library(agate STATIC src/agate/agate.cpp)
target_include_directories(agate PRIVATE src include)
target_link_libraries(agate PRIVATE agate-support)
target_compile_definitions(agate PRIVATE AGT_BUILD_SHARED)


add_library(agate-static STATIC src/agate/agate.cpp)
target_include_directories(agate-static PRIVATE src include)
target_link_libraries(agate-static PRIVATE
        agate-support
        $<TARGET_OBJECTS:agate-static.core>
        $<TARGET_OBJECTS:agate-static.agents>
        $<TARGET_OBJECTS:agate-static.async>
        $<TARGET_OBJECTS:agate-static.channels>
        $<TARGET_OBJECTS:agate-static.threads>)
target_compile_definitions(agate-static PRIVATE AGT_BUILD_STATIC)








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


add_library(DUMMY src/dummy.cpp)
target_include_directories(DUMMY PRIVATE include src)