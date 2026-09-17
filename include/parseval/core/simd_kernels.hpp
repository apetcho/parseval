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

static inline void simd_add(
    const double* __restrict__ a,
    const double* __restrict__ b,
    double* __restrict__ out,
    std::size_t size
){
    std::size_t i = 0;
    // Process 2 doubles at a time (SSE2)
    const std::size_t simd_size = (size / 2) * 2;

    for(; i < simd_size; i += 2){
        __m128d va = _mm_loadu_pd(&a[i]);
        __m128d vb = _mm_loadu_pd(&b[i]);
        __m128d vr = _mm_add_pd(va, vb);
        _mm_storeu_pd(&out[i], vr);
    }

    // Handle remaining elements
    for(; i < size; ++i){
        out[i] = a[i] + b[i];
    }
}

// -
static inline void simd_mul(
    const double* __restrict__ a,
    const double* __restrict__ b,
    double* __restrict__ out,
    std::size_t size
){
    std::size_t i = 0;
    const std::size_t SIZE = (size / 2) * 2;

    for(; i < SIZE; i += 2){
        __m128d va = _mm_loadu_pd(&a[i]);
        __m128d vb = _mm_loadu_pd(&b[i]);
        __m128d vr = _mm_mul_pd(va, vb);
        _mm_storeu_pd(&out[i], vr);
    }

    for(; i < size; ++i){
        out[i] = a[i] * b[i];
    }
}


Array matmul_optimized(const Array& lhs, const Array& rhs);


// --------------------------------------------------------------------
}//-*- end::namespace::parseval                                     -*-
// --------------------------------------------------------------------