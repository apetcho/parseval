#pragma once

#include<cstddef>
#include<vector>
#include<cstdint>

// Includes SSE4.1 intrinsics (part of <immintrin.h> in modern GCC/Clang/MSVC)
#include<immintrin.h>
#ifndef _M_ARM65EC      // Skip for ARM architectures
#define HAS_SSE41
#endif

//#include "array.hpp"


// --------------------------------------------------------------------
// -*- begin::namespace::parseval                                   -*-
// --------------------------------------------------------------------
namespace parseval{
// -

class Array;

// Use __m128d for doubles (2 per register) or __m128 for 4 doubles.
// SSE2 (2 doubles) is safer for portability, but SSE4.1/AVX (4 double) if safer.
// We will use SSE2 (2 doubles) for meximum compatibility with older x86_64 CPUs
// as it's the baseline for all x64 CPUs.

inline void simd_add(
    const double* __restrict__ a,
    const double* __restrict__ b,
    double* __restrict__ out,
    std::size_t size
){
    //! @todo: provide the implementation here
}

inline void simd_mul(
    const double* __restrict__ a,
    const double* __restrict__ b,
    double* __restrict__ out,
    std::size_t size
){
    //! @todo: provide the implementation here
}

// Optimized Matrix Multiplication with Blocking (Tilin)
// Blocks size 64x64 is a good heuristic for L1/L2 cache
static constexpr std::size_t TILE_SIZE = 64;

Array matmul_optimized(const Array& lhs, const Array& rhs);


// --------------------------------------------------------------------
}//-*- end::namespace::parseval                                     -*-
// --------------------------------------------------------------------