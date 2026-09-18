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


/*
TEST(ArrayTest, ScalarOperations){
    //! @todo: implement this
}

TEST(ArrayTest, Broadcasting){
    //! @todo: implement this
}

TEST(ArrayTest, BroadcastDivisionByZero){
    //! @todo: implement this
}


// ==================================================================
// 3. Slicing
// ==================================================================

TEST(ArrayTest, Slice1D){
    //! @todo: implement this
}

TEST(ArrayTest, Slice2D){
    //! @todo: implement this
}


// ==================================================================
// 4. Reductions & Trace
// ==================================================================

TEST(ArrayTest, Reductions){
    //! @todo: implement this
}

TEST(ArrayTest, Trace){
    //! @todo: implement this
}


// ==================================================================
// 5. Linear Algebra (Eigen-based)
// ==================================================================

TEST(ArrayDecompositon, SVD){
    //! @todo: implement this
}

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