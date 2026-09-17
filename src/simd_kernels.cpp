#include "parseval/core/simd_kernels.hpp"
#include "parseval/core/array.hpp"


// --------------------------------------------------------------------
// -*- begin::namespace::parseval                                   -*-
// --------------------------------------------------------------------
namespace parseval{
// -

// Optimized Matrix Multiplication with Blocking (Tilin)
// Blocks size 64x64 is a good heuristic for L1/L2 cache
static constexpr std::size_t TILE_SIZE = 64;

// -
Array matmul_optimized(const Array& A, const Array& B){
    if(A.ndim() != 2 || B.ndim() != 2){
        throw ParsevalError("`matmul()` requires 2D arrays");
    }

    if(A.shape()[1] != B.shape()[0]){
        throw ParsevalError("Incompatible dimensions for `matmul()`");
    }

    const std::size_t M = A.shape()[0];     // Rows A
    const std::size_t K = A.shape()[1];     // Cols A / Rows B
    const std::size_t N = B.shape()[1];     // Cols B

    Array C({M, N}, 0.0);

    const double* __restrict__ a_ptr = A.raw_data();
    const double* __restrict__ b_ptr = B.raw_data();
    double* __restrict__ c_ptr = C.raw_data();

    // Tiling: Loop over tiles of C
    for(std::size_t mb=0; mb < M; mb += TILE_SIZE){
        std::size_t me = std::min(mb+TILE_SIZE, M);
        for(std::size_t nb=0; nb < N; nb += TILE_SIZE){
            std::size_t ne = std::min(nb + TILE_SIZE, N);

            // For each tile, accumulate products from K dimension
            for(std::size_t kb=0; kb < K; kb += TILE_SIZE){
                std::size_t ke = std::min(kb + TILE_SIZE, K);

                // Inner loops with SIMD optimization for accumulation
                for(std::size_t i=mb; i < me; ++i){
                    for(std::size_t j=nb; j < ne; ++j){
                        double sum = 0.0;
                        // Inner loop: unrolled manually or left as is
                        // Since we are accumulating into 'sum', SIMD is tricky here
                        // unless we use 2 accumulators.
                        // For simplicity and robustness in this refactor, we keep
                        // the inner loop scalar but rely on block loading to keep
                        // data in L1 cache.

                        // To truly SIMD this, we'd need to load blocks of A and B
                        // into registers and compute partial sums.
                        // However, the standard loop with blocking is usually
                        // sufficient to beat naive implementations by 5-10x due to
                        // cache locality.
                        for(std::size_t k=kb; k < ke; ++k){
                            sum += a_ptr[i*K + k] * b_ptr[k*N + j];
                        }
                        c_ptr[i*N + j] += sum;
                    }
                }
            }
        }
    }

    return C;
}

// --------------------------------------------------------------------
}//-*- end::namespace::parseval                                     -*-
// --------------------------------------------------------------------