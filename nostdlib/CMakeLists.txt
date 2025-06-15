cmake_minimum_required(VERSION 3.20)
project(nostdlib LANGUAGES C)

option(NOC_BUILD_TESTS "Also build unittests" ON)
option(NOC_BUILD_TESTBED "Test exectuable" ON)

set(NOC_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/noc/include)
set(NOC_SOURCES_DIR ${PROJECT_SOURCE_DIR}/noc/src)

add_library(noc
  ${NOC_SOURCES_DIR}/abs.c
  ${NOC_SOURCES_DIR}/buf.c
  ${NOC_SOURCES_DIR}/countdigits.c
  ${NOC_SOURCES_DIR}/from_str.c
  ${NOC_SOURCES_DIR}/memory.c
  ${NOC_SOURCES_DIR}/platform.c
  ${NOC_SOURCES_DIR}/flt_charcount.c
  ${NOC_SOURCES_DIR}/str.c
  ${NOC_SOURCES_DIR}/to_str.c
  ${NOC_SOURCES_DIR}/io.c
  ${NOC_SOURCES_DIR}/http.c

  # ${NOC_SOURCES_DIR}/fmt/sscanf.c

  ${NOC_INCLUDE_DIR}/noc/numeric/flt_charcount.h
  ${NOC_INCLUDE_DIR}/noc/numeric/to_str.h
  ${NOC_INCLUDE_DIR}/noc/numeric/from_str.h
  ${NOC_INCLUDE_DIR}/noc/numeric/abs.h
  ${NOC_INCLUDE_DIR}/noc/numeric/countdigits.h
  ${NOC_INCLUDE_DIR}/noc/memory.h
  ${NOC_INCLUDE_DIR}/noc/version.h
  ${NOC_INCLUDE_DIR}/noc/types.h
  ${NOC_INCLUDE_DIR}/noc/bits.h
  ${NOC_INCLUDE_DIR}/noc/str.h
  ${NOC_INCLUDE_DIR}/noc/char.h
  ${NOC_INCLUDE_DIR}/noc/macros.h
  ${NOC_INCLUDE_DIR}/noc/math/round.h
  ${NOC_INCLUDE_DIR}/noc/math/mod.h
  ${NOC_INCLUDE_DIR}/noc/buf.h
)

target_compile_features(
  noc PRIVATE c_std_11
)

target_include_directories(
  noc PUBLIC ${NOC_INCLUDE_DIR}
)

if(NOC_BUILD_TESTBED)
  add_executable(testbed ${PROJECT_SOURCE_DIR}/testbed.c)
  target_link_libraries(testbed PRIVATE noc)
  target_include_directories(testbed PRIVATE ${PROJECT_SOURCE_DIR}/include)
  target_compile_features(testbed PRIVATE c_std_11)
endif()

# TODO(i.akkuzin): Remove when we finally implement math library by our self [2024/07/06]

if (NOT DEFINED WIN32)
  target_link_libraries(noc PRIVATE m)
endif()

if(NOC_BUILD_TESTS)
    enable_testing()

    foreach(NOC_TEST_SOURCE
        ${PROJECT_SOURCE_DIR}/noc/tests/test_memory.c
    )
        get_filename_component(NOC_TEST_NAME ${NOC_TEST_SOURCE} NAME_WE)
        add_executable(${NOC_TEST_NAME} ${NOC_TEST_SOURCE})

        target_link_libraries(
            ${NOC_TEST_NAME}
            PRIVATE noc
        )

        target_include_directories(
            ${NOC_TEST_NAME}
            PRIVATE ${PROJECT_SOURCE_DIR}/include
        )

        target_compile_features(${NOC_TEST_NAME} PRIVATE c_std_11)

        add_test(${NOC_TEST_NAME} ${NOC_TEST_NAME})
    endforeach()
endif()
