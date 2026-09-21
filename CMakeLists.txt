cmake_minimum_required(VERSION 3.20)
project(itrs_ng VERSION 0.1.0 LANGUAGES C CXX)

option(ITRS_BUILD_TESTS "Build deterministic resolver tests" ON)
option(ITRS_ENABLE_CELIX "Build the Celix Number application and synthetic resource bundles" OFF)
set(ITRS_CELIX_SOURCE_DIR "" CACHE PATH "Optional Apache Celix source checkout used for an in-tree build")

add_library(itrs_number_core STATIC
    src/number/resolver.c
    src/number/enum.c)
target_include_directories(itrs_number_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(itrs_number_core PUBLIC c_std_11)
set_target_properties(itrs_number_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(itrs_number_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_library(itrs_edge_core STATIC
    src/edge/edge.c)
target_include_directories(itrs_edge_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(itrs_edge_core PUBLIC c_std_11)
target_link_libraries(itrs_edge_core PUBLIC itrs_number_core)
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(itrs_edge_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_executable(itrs-number-demo examples/number_demo.c)
target_link_libraries(itrs-number-demo PRIVATE itrs_number_core)

add_executable(itrs-enum-demo examples/enum_demo.c)
target_link_libraries(itrs-enum-demo PRIVATE itrs_number_core)

if (ITRS_BUILD_TESTS)
    enable_testing()
    add_executable(itrs-number-core-test tests/number_resolver_test.c)
    target_link_libraries(itrs-number-core-test PRIVATE itrs_number_core)
    if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(itrs-number-core-test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME number-core COMMAND itrs-number-core-test)

    add_executable(itrs-enum-test tests/enum_test.c)
    target_link_libraries(itrs-enum-test PRIVATE itrs_number_core)
    if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(itrs-enum-test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME enum-core COMMAND itrs-enum-test)

    add_executable(itrs-edge-test tests/edge_test.c)
    target_link_libraries(itrs-edge-test PRIVATE itrs_edge_core)
    if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(itrs-edge-test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME edge-core COMMAND itrs-edge-test)
endif()

if (ITRS_ENABLE_CELIX)
    if (ITRS_CELIX_SOURCE_DIR)
        set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
        set(FRAMEWORK_CURLINIT OFF CACHE BOOL "" FORCE)
        set(BUILD_UTILS ON CACHE BOOL "" FORCE)
        set(BUILD_FRAMEWORK ON CACHE BOOL "" FORCE)
        set(BUILD_LAUNCHER ON CACHE BOOL "" FORCE)
        set(BUILD_RCM OFF CACHE BOOL "" FORCE)
        set(BUILD_CELIX_DFI OFF CACHE BOOL "" FORCE)
        set(BUILD_CELIX_ETCDLIB OFF CACHE BOOL "" FORCE)
        set(CELIX_CXX14 OFF CACHE BOOL "" FORCE)
        set(CELIX_CXX17 OFF CACHE BOOL "" FORCE)
        set(BUILD_PROMISES OFF CACHE BOOL "" FORCE)
        set(BUILD_PUSHSTREAMS OFF CACHE BOOL "" FORCE)
        set(BUILD_HTTP_ADMIN OFF CACHE BOOL "" FORCE)
        set(BUILD_LOG_SERVICE OFF CACHE BOOL "" FORCE)
        set(BUILD_LOG_HELPER OFF CACHE BOOL "" FORCE)
        set(BUILD_SHELL_API OFF CACHE BOOL "" FORCE)
        set(BUILD_SHELL OFF CACHE BOOL "" FORCE)
        set(BUILD_SHELL_TUI OFF CACHE BOOL "" FORCE)
        set(BUILD_SHELL_WUI OFF CACHE BOOL "" FORCE)
        set(BUILD_REMOTE_SHELL OFF CACHE BOOL "" FORCE)
        set(BUILD_REMOTE_SERVICE_ADMIN OFF CACHE BOOL "" FORCE)
        set(BUILD_CXX_REMOTE_SERVICE_ADMIN OFF CACHE BOOL "" FORCE)
        set(BUILD_EVENT_ADMIN OFF CACHE BOOL "" FORCE)
        set(BUILD_COMPONENTS_READY_CHECK OFF CACHE BOOL "" FORCE)
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(BUILD_EXPERIMENTAL OFF CACHE BOOL "" FORCE)
        add_subdirectory("${ITRS_CELIX_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/apache-celix" EXCLUDE_FROM_ALL)
        if (TARGET framework AND CMAKE_C_COMPILER_ID STREQUAL "GNU")
            target_compile_options(framework PRIVATE -Wno-error=maybe-uninitialized)
        endif()
        set(CELIX_CMAKE_DIRECTORY "${ITRS_CELIX_SOURCE_DIR}/cmake/cmake_celix")
    else()
        find_package(Celix REQUIRED)
    endif()

    add_library(itrs_number_service_api INTERFACE)
    target_include_directories(itrs_number_service_api INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/celix/include)
    target_link_libraries(itrs_number_service_api INTERFACE Celix::framework)

    add_celix_bundle(itrs_number_bundle
        VERSION 1.0.0
        SYMBOLIC_NAME org_itrsng_number
        NAME "iTRS NG Tilden Number"
        FILENAME itrs_number
        SOURCES celix/src/number_bundle.c)
    target_link_libraries(itrs_number_bundle PRIVATE itrs_number_core itrs_number_service_api)

    add_celix_bundle(itrs_resource_fixture_bundle
        VERSION 1.0.0
        SYMBOLIC_NAME org_itrsng_fixture_asl_resources
        NAME "iTRS NG synthetic ASL resources"
        FILENAME itrs_resource_fixture
        SOURCES celix/src/resource_fixture_bundle.c)
    target_link_libraries(itrs_resource_fixture_bundle PRIVATE itrs_number_service_api)

    add_celix_bundle(itrs_enum_fixture_bundle
        VERSION 1.0.0
        SYMBOLIC_NAME org_itrsng_fixture_enum_provider
        NAME "iTRS NG synthetic ENUM provider"
        FILENAME itrs_enum_fixture
        SOURCES celix/src/enum_fixture_bundle.c)
    target_link_libraries(itrs_enum_fixture_bundle PRIVATE itrs_number_core itrs_number_service_api)

    add_celix_bundle(itrs_enum_dns_bundle
        VERSION 1.0.0
        SYMBOLIC_NAME org_itrsng_enum_dns_provider
        NAME "iTRS NG system DNS ENUM provider"
        FILENAME itrs_enum_dns
        SOURCES celix/src/enum_dns_bundle.c)
    target_link_libraries(itrs_enum_dns_bundle PRIVATE itrs_number_core itrs_number_service_api resolv)

    add_celix_bundle(itrs_smoke_consumer_bundle
        VERSION 1.0.0
        SYMBOLIC_NAME org_itrsng_fixture_smoke_consumer
        NAME "iTRS NG Number smoke consumer"
        FILENAME itrs_smoke_consumer
        SOURCES celix/src/smoke_consumer_bundle.c)
    target_link_libraries(itrs_smoke_consumer_bundle PRIVATE itrs_number_service_api)

    if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        foreach(itrs_bundle_target IN ITEMS
                itrs_number_bundle
                itrs_resource_fixture_bundle
                itrs_enum_fixture_bundle
                itrs_enum_dns_bundle
                itrs_smoke_consumer_bundle)
            target_compile_options(${itrs_bundle_target} PRIVATE -Wall -Wextra -Werror -Wno-unused-parameter)
        endforeach()
    endif()

    add_celix_container(itrs-ng-celix-demo COPY BUNDLES
        itrs_number_bundle
        itrs_resource_fixture_bundle
        itrs_enum_fixture_bundle
        itrs_enum_dns_bundle
        itrs_smoke_consumer_bundle)
endif()
