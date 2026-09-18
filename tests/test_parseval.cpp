#include<gtest/gtest.h>
#include<fstream>
#include<vector>

#include<cmath>
#include<map>

#include "parseval/parseval.hpp"

using parseval::Array;
using parseval::ThreadPool;

// ==================================================================
// 1. Basic Array Creation & Access
// ==================================================================

TEST(ArrayTest, BasicCreationAndAccess){
    // 2x3 array filled with 1.0
    Array arr({2, 3}, 01.0);

    EXPECT_EQ(arr.ndim(), 2);
    EXPECT_EQ(arr.shape(), Array::Shape({2, 3}));
    EXPECT_DOUBLE_EQ(arr(0, 0), 1.0);
    EXPECT_DOUBLE_EQ(arr(1, 2), 1.0);

    // Modify element
    arr(0, 1) = 2.5;
    EXPECT_DOUBLE_EQ(arr(0, 1), 2.5);

    // Safe access with bound checking
    EXPECT_THROW(arr.at({2, 3}), parseval::ParsevalError);
}


TEST(ArrayTest, FactoryMethods){
    Array zeros = Array::zeros({3, 3});
    EXPECT_DOUBLE_EQ(zeros(0, 0), 0.0);

    Array ones = Array::ones({2, 2});
    EXPECT_DOUBLE_EQ(ones(1, 1), 1.0);

    Array full = Array::full({2}, 5.5);
    EXPECT_DOUBLE_EQ(full(0), 5.5);

    Array arange = Array::arange(0.0, 3.0, 1.0);
    EXPECT_EQ(arange.size(), 3);
    EXPECT_DOUBLE_EQ(arange(0), 0.0);
    EXPECT_DOUBLE_EQ(arange(2), 2.0);

    Array random = Array::random(Array::Shape({2, 2}));
    EXPECT_LE(random(0, 0), 1.0);
}


// ==================================================================
// 2. Elementwise Operations & Broadcasting
// ==================================================================

TEST(ArrayTest, ElementwiseOperations){
    Array A({2, 2}, {1.0, 2.0, 3.0, 4.0});
    Array B({2, 2}, {5.0, 6.0, 7.0, 8.0});

    Array sum = A + B;
    EXPECT_DOUBLE_EQ(sum(0, 0), 6.0);

    Array diff = A - B;
    EXPECT_DOUBLE_EQ(diff(0, 0), -4.0);

    Array prod = A * B;
    EXPECT_DOUBLE_EQ(prod(0, 0), 5.0);

    Array quot = A / B;
    EXPECT_DOUBLE_EQ(quot(0, 0), 0.2);
}

TEST(ArrayTest, ScalarOperations){
    Array A({3}, {1.0, 2.0, 3.0});
    Array B = A + 10.0;
    EXPECT_DOUBLE_EQ(B(0), 11.0);

    Array C = A * 2.0;
    EXPECT_DOUBLE_EQ(C(2), 6.0);

    Array D = A / 2.0;
    EXPECT_DOUBLE_EQ(D(0), 0.5);
}

TEST(ArrayTest, Broadcasting){
    // Shape (3, 1) + (1, 4) -> (3, 4)
    Array A({3, 1}, {1.0, 2.0, 3.0});
    Array B({1, 4}, {10.0, 20.0, 30.0, 40.0});

    Array C = A + B;

    EXPECT_EQ(C.shape(), Array::Shape({3, 4}));
    EXPECT_DOUBLE_EQ(C(0, 0), 11.0);
    EXPECT_DOUBLE_EQ(C(2, 3), 43.0);
}

TEST(ArrayTest, BroadcastDivisionByZero){
    Array A({2, 2}, {1.0, 2.0, 3.0, 4.0});
    Array B({1, 2}, {0.0, 1.0});            // First row is 0

    EXPECT_THROW(A / B, parseval::ParsevalError);
}


// ==================================================================
// 3. Slicing
// ==================================================================

TEST(ArrayTest, Slice1D){
    Array A({10}, {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0});
    Array slice = A.slice(2, 8, 2);     // [2, 4, 6]

    EXPECT_EQ(slice.size(), 3);
    EXPECT_DOUBLE_EQ(slice(0), 2.0);
    EXPECT_DOUBLE_EQ(slice(2), 6.0);
}

TEST(ArrayTest, Slice2D){
    Array matrix({4, 5}, {
        0.0, 1.0, 2.0, 3.0, 4.0,
        5.0, 6.0, 7.0, 8.0, 9.0,
        10.0, 11.0, 12.0, 13.0, 14.0,
        15.0, 16.0, 17.0, 18.0, 19.0
    });

    Array slice = matrix.slice(1, 3, 1, 4, 1, 1);   // Rows 1-3, Cols 1-4

    EXPECT_EQ(slice.shape(), Array::Shape({2, 3}));
    EXPECT_DOUBLE_EQ(slice(0, 0), 6.0);
    EXPECT_DOUBLE_EQ(slice(1, 2), 13.0);
}

// ==================================================================
// 4. Reductions & Trace
// ==================================================================

TEST(ArrayTest, Reductions){
    Array A({2, 3}, {1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
    EXPECT_DOUBLE_EQ(A.sum(), 21.0);
    EXPECT_DOUBLE_EQ(A.min(), 1.0);
    EXPECT_DOUBLE_EQ(A.max(), 6.0);
}

TEST(ArrayTest, Trace){
    Array matrix({3, 3}, {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0});
    EXPECT_DOUBLE_EQ(matrix.trace(), 15.0); // 1 + 5 + 9

    EXPECT_THROW(matrix.trace(), parseval::ParsevalError); // 1D array
}

// ==================================================================
// 5. Linear Algebra (Eigen-based)
// ==================================================================

TEST(ArrayDecompositon, SVD){
    Array matrix({2, 2}, {3.0, 0.0, 0.0, 2.0});
    Array U, S, Vt;
    matrix.svd(U, S, Vt);

    EXPECT_EQ(U.shape(), Array::Shape({2, 2}));
    EXPECT_EQ(S.shape(), Array::Shape({2}));
    EXPECT_DOUBLE_EQ(S(0), 3.0);
    EXPECT_DOUBLE_EQ(S(1), 2.0);
}

/*
TEST(ArrayDecomposition, QR){
    //! @todo: implement this
}

TEST(ArrayDecomposition, LU){
    //! @todo: implement this
}

TEST(ArrayDecomposition, Cholesky){
    //! @todo: implement this
}


TEST(ArrayDecomposition, Eigen){
    //! @todo: implement this
}

TEST(ArrayDecomposition, Rank){
    //! @todo: implement this
}

TEST(ArrayDecomposition, DetAndInverse){
    //! @todo: implement this
}

TEST(ArrayDecomposition, Diag){
    //! @todo: implement this
}


// ==================================================================
// 6. Functional Primitives & Unary Math
// ==================================================================

TEST(ArrayFunctional, UnaryMath){
    //! @todo: implement this
}

TEST(ArrayFunctional, MapAndApply){
    //! @todo: implement this
}

TEST(ArrayFunctional, AnyAndAll){
    //! @todo: implement this
}


// ==================================================================
// 7. I/O: Text, Binary, CSV, SQLite
// ==================================================================

TEST(ArrayIO, TextIO){
    //! @todo: implement this
}

TEST(ArrayIO, BinaryIO){
    //! @todo: implement this
}

TEST(ArrayIO, CSVIO){
    //! @todo: implement this
}

TEST(ArrayIO, SQLiteIO){
    //! @todo: implement this
}


// ==================================================================
// 8. NetCDF: Attributes, Compression, Multiple Variables, Geospatial
// ==================================================================

TEST(ArrayNetCDF, Attributes){
    //! @todo: implement this
}

TEST(ArrayNetCDF, Compression){
    //! @todo: implement this
}

TEST(ArrayNetCDF, MultipleVariables){
    //! @todo: implement this
}

TEST(ArrayNetCDF, Geospatial){
    //! @todo: implement this
}


// ==================================================================
// 9. ThreadPool & Parallelism
// ==================================================================

TEST(ThreadPoolTest, ExecuteTasks){
    //! @todo: implement this
}

TEST(ArrayParallel, LargeMatrixMult){
    //! @todo: implement this
}

TEST(ArrayParallel, LargeElementwise){
    //! @todo: implement this
}


// ==================================================================
// Main Entry Point
// ==================================================================

int main(int argc, char** argv){
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}



*/