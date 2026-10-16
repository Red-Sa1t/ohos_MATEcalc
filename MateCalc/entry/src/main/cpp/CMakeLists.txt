# MATE Calculator native module for HarmonyOS
cmake_minimum_required(VERSION 3.5.0)
project(matecalc C CXX)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
# Prebuilt GMP / MPFR / MPC (see third_party/build-mp.sh)
set(MP_PREBUILT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../third_party/prebuilt/${OHOS_ARCH})

if(NOT EXISTS ${MP_PREBUILT_ROOT}/lib/libmpc.a)
    message(FATAL_ERROR "Missing ${MP_PREBUILT_ROOT}; run third_party/build-mp.sh ${OHOS_ARCH}")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_library(mp_gmp STATIC IMPORTED)
set_target_properties(mp_gmp PROPERTIES IMPORTED_LOCATION ${MP_PREBUILT_ROOT}/lib/libgmp.a)
add_library(mp_mpfr STATIC IMPORTED)
set_target_properties(mp_mpfr PROPERTIES IMPORTED_LOCATION ${MP_PREBUILT_ROOT}/lib/libmpfr.a)
add_library(mp_mpc STATIC IMPORTED)
set_target_properties(mp_mpc PROPERTIES IMPORTED_LOCATION ${MP_PREBUILT_ROOT}/lib/libmpc.a)

set(CORE_SOURCES
    core/mp.c
    core/mp-binary.c
    core/mp-convert.c
    core/mp-trigonometric.c
    core/mp-equation.c
    core/mp-serializer.c
    core/parser.c
    core/parserfunc.c
    core/lexer.c
    core/prelexer.c
    core/financial.c
    core/unit.c
    core/unit-category.c
    core/unit-manager.c
    core/currency.c
    core/currency-manager.c
    core/math-variables.c
    compat/glib-compat.c
    engine/calc-engine.c
)

add_library(matecalc SHARED napi_calc.cpp ${CORE_SOURCES})
target_include_directories(matecalc PRIVATE
    ${NATIVERENDER_ROOT_PATH}/compat
    ${NATIVERENDER_ROOT_PATH}/core
    ${NATIVERENDER_ROOT_PATH}/engine
    ${MP_PREBUILT_ROOT}/include
)
target_compile_definitions(matecalc PRIVATE MATE_CALC_OHOS=1)
target_compile_options(matecalc PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wno-unused-parameter>)
target_link_libraries(matecalc PUBLIC libace_napi.z.so libhilog_ndk.z.so mp_mpc mp_mpfr mp_gmp)
