#include "parseval/parseval.hpp"

#include<Eigen/Cholesky>
#include<Eigen/Eigenvalues>
#include<Eigen/LU>
#include<Eigen/QR>
#include<Eigen/SVD>

#include<sqlite3.h>
#include<sqlite3ext.h>

#include<algorithm>
#include<sstream>
#include<fstream>
#include<numeric>
#include<utility>
#include<cctype>
#include<cmath>

// --------------------------------------------------------------------
// -*- begin::namespace::parseval                                   -*-
// --------------------------------------------------------------------
namespace parseval{
// -

//! @brief: Return the number of element this array instance can hold
std::size_t Array::product(const Shape& shape){
    if(shape.empty()){ return 1; }

    return std::accumulate(
        shape.begin(), shape.end(),
        std::size_t{1},
        std::multiplies<std::size_t>{}
    );
}

//
void Array::compute_strides(void){
    this->m_strides.assign(this->m_shape.size(), 1);

    if(this->m_shape.empty()){ return; }

    for(std::size_t i=this->m_shape.size()-1; i > 0; --i){
        this->m_strides[i-1] = this->m_strides[i] * this->m_shape[i];
    }
}

//
std::size_t Array::offset(const Shape& indices) const{
    if(indices.size() != this->m_shape.size()){
        throw ParsevalError("Number of idinces must match array rank");
    }

    std::size_t result = 0;
    for(std::size_t i=0; i < indices.size(); ++i){
        if(indices[i] >= this->m_shape[i]){
            throw ParsevalError("Array index out of bounds");
        }

        result += indices[i] * this->m_strides[i];
    }

    return result;
}

// -
void Array::validate_same_shape(const Array& other) const{
    if(this->m_shape != other.m_shape){
        throw ParsevalError("Elementwise operation requires equal shapes");
    }
}

// -------------------
// -*- Contructors -*-
// -------------------
Array::Array(const Shape& shape, value_type value)
: m_shape{shape}
, m_data(this->product(shape), value)
{
    this->compute_strides();
}

// -
Array::Array(std::initializer_list<std::size_t> shape, value_type value)
: Array(Shape(shape), value)
{}

// -
Array::Array(const Shape& shape, const std::vector<value_type>& data)
: m_shape{shape}
, m_data(data)
{
    if(this->m_data.size() != this->product(this->m_shape)){
        throw ParsevalError("Data size does not match array shape");
    }

    this->compute_strides();
}


// -
Array::Array(const Shape& shape, std::vector<value_type>&& data)
: m_shape(shape)
, m_data(std::move(data))
{
    if(this->m_data.size() != this->product(this->m_shape)){
        throw ParsevalError("Data size does not match array shape");
    }

    this->compute_strides();
}

// --------------------------------
// - Factory methods return Array -
// --------------------------------
Array Array::zeros(const Shape& shape){
    return Array(shape, 0.0);
}

// -
Array Array::ones(const Shape& shape){
    return Array(shape, 1.0);
}


// -
Array Array::identity(size_t n){
    Array::Shape shape = {n, n};
    auto result = Array::zeros(shape);
    for(std::size_t i=0; i < n; ++i){
        result(i, i) = 1.0;
    }

    return result;
}

// -
Array Array::full(const Shape& shape, value_type value){
    return Array(shape, value);
}

// -
Array Array::arange(value_type start, value_type stop, value_type step){
    if(step==0.0){
        throw ParsevalError("");
    }

    std::vector<value_type> values{};

    if(step > 0.0){
        for(value_type x=start; x < stop; x += step){
            values.push_back(x);
        }
    }else{
        for(value_type x=start; x > stop; x += step){
            values.push_back(x);
        }
    }

    return Array({values.size()}, std::move(values));
}

// -
std::size_t Array::ndim(void) const noexcept{
    return this->m_shape.size();
}

// -
const Array::Shape& Array::shape(void) const noexcept{
    return this->m_shape;
}

// -
const Array::Shape& Array::stride(void) const noexcept{
    return this->m_strides;
}

// -
std::size_t Array::size(void) const noexcept{
    return this->m_data.size();
}


// -
bool Array::empty(void) const noexcept{
    return this->m_data.empty();
}

// -
const std::vector<Array::value_type>& Array::data(void) const noexcept{
    return this->m_data;
}

// -
std::vector<Array::value_type>& Array::data(void) noexcept{
    return this->m_data;
}

// -
const Array::value_type* Array::raw_data(void) const noexcept{
    return this->m_data.data();
}

// -
Array::value_type* Array::raw_data(void) noexcept{
    return this->m_data.data();
}

// -
Array::value_type& Array::at(const Shape& indices){
    return this->m_data[this->offset(indices)];
}

// -*-
const Array::value_type& Array::at(const Shape& indices) const{
    return this->m_data[this->offset(indices)];
}

// -
template<typename... Indices>
Array::value_type& Array::operator()(Indices... indices){
    return this->at(Array::Shape{
        static_cast<std::size_t>(indices)...
    });
}

// -
template<typename... Indices>
const Array::value_type& Array::operator()(Indices... indices) const{
    return this->at(Array::Shape{
        static_cast<std::size_t>(indices)...
    });
}

// -
Array Array::reshape(const Shape& myNewShape) const{
    if(this->product(myNewShape) != this->size()){
        throw ParsevalError("Reshape cannot change the number of elements.");
    }

    return Array(myNewShape, this->m_data);
}

// -
Array Array::transpose(void) const{
    if(this->ndim() != 2){
        throw ParsevalError("`transpose()` requires a two-dimensional array.");
    }

    const std::size_t rows = this->m_shape[0];
    const std::size_t cols = this->m_shape[1];

    Array result({cols, rows});
    for(std::size_t r=0; r < rows; ++r){
        for(std::size_t c=0; c < cols; ++c){
            result(c, r) = (*this)(r, c);
        }
    }

    return result;
}

// -
Array Array::matmul(const Array& other) const{
    if(this->ndim() != 2 || other.ndim() != 2){
        throw ParsevalError(
            "`matmul()` currently supports two-dimensional arrays only"
        );
    }

    const std::size_t rows = this->m_shape[0];
    const std::size_t inner = this->m_shape[1];
    const std::size_t other_rows = other.m_shape[0];
    const std::size_t cols = other.m_shape[1];

    if(inner != other_rows){
        throw ParsevalError(
            "Matrix dimensions are incompatible for multiplication"
        );
    }

    Array result({rows, cols}, 0.0);

    // Estimate the total arithmetic work. The helper itself
    // partitions work by output row.
    const std::size_t op_count = rows * inner * cols;

    auto multiply_rows = [this, &other, &result, inner, cols](std::size_t begin_row, std::size_t end_row){
        for(std::size_t row=begin_row; row < end_row; ++row){
            for(std::size_t col=0; col < cols; ++col){
                double val{0.0};

                for(std::size_t p=0; p < inner; ++p){
                    val += (*this)(row, p) * other(p, col);
                }

                result(row, col) = val;
            }
        }
    };

    // For matrix multiplication, the amount of work per row can be large.
    // Therefore, use one task per row partition whenever the total operation
    // count is sufficientily large.
    const std::size_t minimum_parallel_rows = (
        (op_count > Array::parallel_threshold) ? 1 : rows + 1
    );
    this->parallel_for(
        rows,
        multiply_rows,
        minimum_parallel_rows
    );

    return result;
}

// -
Array::value_type Array::sum(void) const{
    return std::accumulate(this->m_data.begin(), this->m_data.end(), 0.0);
}

// -
Array::value_type Array::min(void) const{
    if(this->empty()){
        throw ParsevalError("`min` is undefined for an empty array");
    }

    return *std::min_element(this->m_data.begin(), this->m_data.end());
}

// -
Array::value_type Array::max(void) const{
    if(this->empty()){
        throw ParsevalError("`min` is undefined for an empty array");
    }

    return *std::max_element(this->m_data.begin(), this->m_data.end());
}

// -
Array Array::operator+(const Array& other) const{
    // return this->elementwise_binary(
    //     other,
    //     [](Array::value_type lhs, Array::value_type rhs){
    //         return lhs + rhs;
    //     }
    // );
    return this->elementwise_binary_broadcast(
        other,
        [](value_type x, value_type y){ return x + y; }
    );
}

// -
Array Array::operator-(const Array& other) const{
    // return this->elementwise_binary(
    //     other,
    //     [](Array::value_type lhs, Array::value_type rhs){
    //         return lhs - rhs;
    //     }
    // );
    return this->elementwise_binary_broadcast(
        other,
        [](value_type x, value_type y){ return x - y; }
    );
}

// -
Array Array::operator*(const Array& other) const{
    // return this->elementwise_binary(
    //     other,
    //     [](Array::value_type lhs, Array::value_type rhs){
    //         return lhs * rhs;
    //     }
    // );
    return this->elementwise_binary_broadcast(
        other,
        [](value_type x, value_type y){ return x * y; }
    );
}

// -
Array Array::operator/(const Array& other) const{
    // this->validate_same_shape(other);
    // Array result(this->m_shape);
    // for(std::size_t i=0; i < this->size(); ++i){
    //     if(other.m_data[i] == 0.0){
    //         throw ParsevalError("Division by zero");
    //     }
    // }

    // return this->elementwise_binary(
    //     other,
    //     [](Array::value_type lhs, Array::value_type rhs){
    //         return lhs / rhs;
    //     }
    // );

    // For broadcasting, we can't pre-check all zeros globally; check on the fly
    return this->elementwise_binary_broadcast(
        other,
        [](value_type x, value_type y){
            if(y==0.0){
                throw ParsevalError("Division by zero");
            }
            return x / y;
        }
    );
}

// -
Array Array::operator+(value_type scalar) const{
    return this->elementwise_scalar(
        scalar,
        [](Array::value_type val, Array::value_type scalar_value){
            return val + scalar_value;
        }
    );
}

// -
Array Array::operator-(value_type scalar) const{
    return this->elementwise_scalar(
        scalar,
        [](Array::value_type val, Array::value_type scalar_value){
            return val - scalar_value;
        }
    );
}

// -
Array Array::operator*(value_type scalar) const{
    return this->elementwise_scalar(
        scalar,
        [](Array::value_type val, Array::value_type scalar_value){
            return val * scalar_value;
        }
    );
}

// -
Array Array::operator/(value_type scalar) const{
    if(scalar==0.0){
        throw ParsevalError("Division by zero");
    }
    return this->elementwise_scalar(
        scalar,
        [](Array::value_type val, Array::value_type scalar_value){
            return val / scalar_value;
        }
    );
}

// -
Array& Array::operator+=(const Array& other){
    this->validate_same_shape(other);

    this->parallel_for(
        this->size(),
        [this, &other](std::size_t begin, std::size_t end){
            for(std::size_t i=begin; i < end; ++i){
                this->m_data[i] += other.m_data[i];
            }
        }
    );

    return *this;
}

// -
Array& Array::operator-=(const Array& other){
    this->validate_same_shape(other);

    this->parallel_for(
        this->size(),
        [this, &other](std::size_t begin, std::size_t end){
            for(std::size_t i=begin; i < end; ++i){
                this->m_data[i] -= other.m_data[i];
            }
        }
    );

    return *this;
}

// -
Array& Array::operator*=(const Array& other){
    this->validate_same_shape(other);

    this->parallel_for(
        this->size(),
        [this, &other](std::size_t begin, std::size_t end){
            for(std::size_t i=begin; i < end; ++i){
                this->m_data[i] *= other.m_data[i];
            }
        }
    );

    return *this;
}

// -
Array& Array::operator/=(const Array& other){
    this->validate_same_shape(other);

    for(std::size_t i=0; i < this->size(); ++i){
        if(other.m_data[i]==0.0){
            throw ParsevalError("Division by zero");
        }
    }

    this->parallel_for(
        this->size(),
        [this, &other](std::size_t begin, std::size_t end){
            for(std::size_t i=begin; i < end; ++i){
                this->m_data[i] /= other.m_data[i];
            }
        }
    );

    return *this;
}

// -
Array& Array::operator+=(value_type scalar){
    this->parallel_for(
        this->size(),
        [this, scalar](std::size_t begin, std::size_t end){
            for(std::size_t i=begin; i < end; ++i){
                this->m_data[i] += scalar;
            }
        }
    );

    return *this;
}

Array& Array::operator-=(value_type scalar){
    this->parallel_for(
        this->size(),
        [this, scalar](std::size_t begin, std::size_t end){
            for(std::size_t i=begin; i < end; ++i){
                this->m_data[i] -= scalar;
            }
        }
    );

    return *this;
}

// -
Array& Array::operator*=(value_type scalar){
    this->parallel_for(
        this->size(),
        [this, scalar](std::size_t begin, std::size_t end){
            for(std::size_t i=begin; i < end; ++i){
                this->m_data[i] *= scalar;
            }
        }
    );

    return *this;
}

// -
Array& Array::operator/=(value_type scalar){
    if(scalar == 0.0){
        throw ParsevalError("Division by zero");
    }
    this->parallel_for(
        this->size(),
        [this, scalar](std::size_t begin, std::size_t end){
            for(std::size_t i=begin; i < end; ++i){
                this->m_data[i] /= scalar;
            }
        }
    );

    return *this;
}

// -
bool Array::is_close(const Array& other, value_type tolerance) const{
    if(this->m_shape != other.m_shape){
        return false;
    }

    for(std::size_t i=0; i < this->size(); ++i){
        if(std::abs(this->m_data[i] - other.m_data[i]) > tolerance){
            return false;
        }
    }

    return true;
}


// -------------------------
// -*- friends functions -*-
// -------------------------

Array operator+(Array::value_type scalar, const Array& rhs){
    return rhs + scalar;
}

// -
Array operator-(Array::value_type scalar, const Array& rhs){
    return Array::full(rhs.shape(), scalar) - rhs;
}

// -
Array operator*(Array::value_type scalar, const Array& rhs){
    return rhs * scalar;
}

// -*-
Array operator/(Array::value_type scalar, const Array& rhs){
    return Array::full(rhs.shape(), scalar) / rhs;
}

// -
std::ostream& operator<<(std::ostream& stream, const Array& array){
    stream << "Array(shape=[";
    for(std::size_t i=0; i < array.m_shape.size(); ++i){
        if(i > 0){ stream << ", "; }
        stream << array.m_shape[i];
    }
    stream << "], data=[";
    for(std::size_t i=0; i < array.m_data.size(); ++i){
        if(i > 0){ stream << ", "; }
        stream << array.m_data[i];
    }
    stream << "])";
    return stream;
}


// -
template<typename Function>
void Array::parallel_for(
    std::size_t count,
    Function&& function,
    std::size_t minimum_parallel_work=parallel_threshold
) const{
    // -
    if(count==0){ return; }

    ThreadPool& pool = global_thread_pool();

    // Run small operation synchronously. Creating and scheduling
    // tasks can cost more than the computation itself.

    if(count < minimum_parallel_work || pool.size() <= 1){
        function(0, count);
        return;
    }

    const std::size_t task_count = std::min(count, pool.size());
    const std::size_t chunk_size = (count + task_count - 1)/task_count;

    // Store one shared callable so every so every submitter task can
    // safely invoke the same operation.

    using Func = std::decay_t<Function>;
    auto shared_func = std::make_shared<Func>(std::forward<Func>(function));

    std::vector<std::future<void>> futures{};
    futures.reserve(task_count);

    for(std::size_t task_index=0; task_count < task_count; ++task_index){
        const std::size_t begin = task_index * chunk_size;
        const std::size_t end = std::min(count, begin + chunk_size);
        if(begin >= end){ break; }

        futures.push_back(pool.submit(
            [shared_func, begin, end](){
                (*shared_func)(begin, end)
            }
        ));
    }

    // get() waits for completion and propagates worker exceptions
    for(auto& future: futures){
        future.get();
    }
}

// -
template<typename BinaryOp>
Array Array::elementwise_binary(const Array& other, BinaryOp&& func) const{
    this->validate_same_shape(other);

    Array result(this->m_shape);
    this->parallel_for(
        this->size(),
        [this, &other, &result, func](std::size_t begin, std::size_t end){
            for(std::size_t i=begin; i < end; ++i){
                result.m_data[i] = func(this->m_data[i], other.m_data[i]);
            }
        }
    );

    return result;
}

// -
template<typename ScalarOp>
Array Array::elementwise_scalar(value_type scalar, ScalarOp&& func) const{
    Array result(this->m_shape);

    this->parallel_for(
        this->size(),
        [this, &result, scalar, func](std::size_t begin, std::size_t end){
            for(std::size_t i=begin; i < end; ++i){
                result.m_data[i] = func(this->m_data[i], scalar);
            }
        }
    );

    return result;
}

// -------------------------------------------------
// -*- Utility method for handling Eigen library -*-
// -------------------------------------------------

void Array::require_matrix(const char* operation) const{
    if(this->ndim() != 2){
        throw ParsevalError(std::string(operation) + " requires a two-dimensional array");
    }
}

// -
void Array::require_square_matrix(const char* operation) const{
    this->require_matrix(operation);
    if(this->m_shape[0] != this->m_shape[1]){
        throw ParsevalError(std::string(operation) + " requires a square matrix");
    }
}

// -
Eigen::MatrixXd Array::to_eigen_matrix(void) const{
    this->require_matrix("Matrix conversion");

    Eigen::MatrixXd matrix(
        static_cast<Eigen::Index>(this->m_shape[0]),
        static_cast<Eigen::Index>(this->m_shape[1])
    );

    for(std::size_t row=0; row < this->m_shape[0]; ++row){
        for(std::size_t col=0; col < this->m_shape[1]; ++col){
            matrix(
                static_cast<Eigen::Index>(row),
                static_cast<Eigen::Index>(col)
            ) = (*this)(row, col);
        }
    }

    return matrix;
}

// -
Array Array::from_eigen_matrix(const Eigen::MatrixXd& matrix){
    const std::size_t rows = static_cast<std::size_t>(matrix.rows());
    const std::size_t cols = static_cast<std::size_t>(matrix.cols());

    std::vector<Array::value_type> values{};
    values.reserve(rows * cols);
    for(std::size_t row=0; row < rows; ++row){
        for(std::size_t col=0; col < cols; ++col){
            auto i = static_cast<Eigen::Index>(row);
            auto j = static_cast<Eigen::Index>(col);
            values.push_back(matrix(i, j));
        }
    }

    return Array(Array::Shape{rows, cols}, std::move(values));
}

// -
Array Array::from_eigen_vector(const Eigen::VectorXd& vec){
    std::vector<Array::value_type> values{};
    values.reserve(static_cast<std::size_t>(vec.size()));

    for(Eigen::Index i=0; i < vec.size(); ++i){
        values.push_back(vec(i));
    }

    return Array(
        Array::Shape{static_cast<std::size_t>(vec.size())},
        std::move(values)
    );
}

// ----------------------------
// -*- Matrix Decomposition -*-
// ----------------------------
void Array::svd(Array& u, Array& singular_values, Array& vt) const{
    this->require_matrix("svd");
    const auto matrix = this->to_eigen_matrix();
    Eigen::JacobiSVD<Eigen::MatrixXd> decomposition(
        matrix,
        Eigen::ComputeThinU | Eigen::ComputeThinV
    );

    u = Array::from_eigen_matrix(decomposition.matrixU());
    singular_values = this->from_eigen_vector(decomposition.singularValues());
    vt = Array::from_eigen_matrix(decomposition.matrixV().transpose());
}

// -
void Array::qr(Array& q, Array& r) const{
    this->require_matrix("qr");

    auto matrix = this->to_eigen_matrix();
    Eigen::HouseholderQR<Eigen::MatrixXd> decomposition(matrix);
    const Eigen::Index rows = matrix.rows();
    const Eigen::Index cols = matrix.cols();

    Eigen::MatrixXd q_matrix = (
        decomposition.householderQ() *
        Eigen::MatrixXd::Identity(rows, cols)
    );

    Eigen::MatrixXd r_matrix = Eigen::MatrixXd::Zero(rows, cols);
    const Eigen::MatrixXd qr_matrix = decomposition.matrixQR();
    for(Eigen::Index row=0; row < rows; ++row){
        for(Eigen::Index col=row; col < cols; ++col){
            r_matrix(row, col) = qr_matrix(row, col);
        }
    }

    q = Array::from_eigen_matrix(q_matrix);
    r = Array::from_eigen_matrix(r_matrix);
}

// -
void Array::lu(Array& l, Array& u, Array& p) const{
    this->require_square_matrix("lu");
    const Eigen::MatrixXd matrix = this->to_eigen_matrix();

    Eigen::PartialPivLU<Eigen::MatrixXd> decomposition(matrix);

    const Eigen::Index n = matrix.rows();
    // const Eigen::MatrixXd packed = decomposition.matrixLU();
    Eigen::MatrixXd l_matrix = Eigen::MatrixXd::Identity(n, n);
    l_matrix.triangularView<Eigen::Lower>() = decomposition.matrixLU();
    Eigen::MatrixXd u_matrix = decomposition.matrixLU().triangularView<Eigen::Upper>();

    Eigen::MatrixXd p_matrix = decomposition.permutationP();

    l = Array::from_eigen_matrix(l_matrix);
    u = Array::from_eigen_matrix(u_matrix);
    p = Array::from_eigen_matrix(p_matrix);
}

// -
Array Array::cholesky(void) const{
    this->require_square_matrix("cholesky");

    const Eigen::MatrixXd matrix = this->to_eigen_matrix();

    Eigen::LLT<Eigen::MatrixXd> decomposition(matrix);
    if(decomposition.info() != Eigen::Success){
        throw ParsevalError(
            "Cholesky decomposition requiresa symmetric positive-definite matrix"
        );
    }

    return Array::from_eigen_matrix(decomposition.matrixL());
}

// -
void Array::eigen(Array& eigenValues, Array& eigenVectors) const{
    this->require_square_matrix("eigen");

    const Eigen::MatrixXd matrix = this->to_eigen_matrix();

    constexpr double symmetry_tolerance = 1e-12;

    if(!matrix.isApprox(matrix.transpose(), symmetry_tolerance)){
        throw ParsevalError("eigen currently require a real symmetrix matrix");
    }

    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> decomposition(matrix);

    if(decomposition.info() != Eigen::Success){
        throw ParsevalError("Eigenvalue decomposition failed");
    }

    eigenValues = Array::from_eigen_vector(decomposition.eigenvalues());
    eigenVectors = Array::from_eigen_matrix(decomposition.eigenvectors());
}

// -
Array Array::diag(void) const{
    if(this->ndim() == 1){
        const std::size_t n = this->m_shape[0];
        Array result(Array::Shape{n, n}, 0.0);

        for(std::size_t i=0; i < n; i++){
            result(i, i) = this->m_data[i];
        }

        return result;
    }

    if(this->ndim() == 2){
        const std::size_t n = std::min(this->m_shape[0], this->m_shape[1]);

        Array result(Array::Shape{n}, 0.0);
        for(std::size_t i=0; i < n; ++i){
            result(i) = (*this)(i, i);
        }

        return result;
    }

    throw ParsevalError("diag require one- or two-dimensional array");
}

// -
std::size_t Array::rank(void) const{
    this->require_matrix("rank");

    const Eigen::MatrixXd matrix = this->to_eigen_matrix();

    Eigen::JacobiSVD<Eigen::MatrixXd> decomposition(matrix);

    return static_cast<std::size_t>(decomposition.rank());
}

// -
Array::value_type Array::det(void) const{
    this->require_square_matrix("det");
    return this->to_eigen_matrix().determinant();
}

Array Array::inverse(void) const{
    this->require_square_matrix("inverse");

    const Eigen::MatrixXd matrix = this->to_eigen_matrix();
    Eigen::FullPivLU<Eigen::MatrixXd> decomposition(matrix);

    if(!decomposition.isInvertible()){
        throw ParsevalError("Cannot invert a singular matrix");
    }

    return Array::from_eigen_matrix(decomposition.inverse());
}

// -
Array::value_type Array::trace(void) const{
    this->require_square_matrix("trace");

    const std::size_t n = this->m_shape[0];
    Array::value_type result{0.0};

    for(std::size_t i=0; i < n; ++i){
        result += (*this)(i, i);
    }

    return result;
}

// -
Array::Shape Array::broadcast_shape(const Array::Shape& a, const Array::Shape& b){
    if(a.empty() && b.empty()){
        return {};
    }

    const std::size_t na = a.size();
    const std::size_t nb = b.size();
    const std::size_t nd = std::max(na, nb);

    Shape result(nd);

    for(std::size_t i=0; i < nd; ++i){
        std::size_t a_dim = (na > 0) ? a[na - 1 - i] : 1;
        std::size_t b_dim = (nb > 0) ? b[nb - 1 - i] : 1;

        if(a_dim==1 || b_dim==1 || a_dim==b_dim){
            result[nd - 1 - i] = std::max(a_dim, b_dim);
        }else{
            std::stringstream ss;
            ss << "Shapes " << Array::to_string(a);
            ss << " and " << Array::to_string(b) << " are not broadcastable";

            throw ParsevalError(ss.str());
        }
    }

    return result;
}

// -
Array::Shape Array::broadcast_strides(const Array::Shape& shape, const Array::Shape& strides){
    // Compute strides for row-major
    Array::Shape my_shape(shape.size());
    my_shape[shape.size() - 1] = 1;
    for(std::size_t i=shape.size()-1; i > 0; --i){
        my_shape[i-1] = shape[i] * my_shape[i];
    }

    return my_shape;
}

// -
std::string Array::to_string(const Array::Shape& shape){
    std::stringstream ss;
    ss << "[";
    for(std::size_t i=0; i < shape.size(); ++i){
        if(i > 0){ ss << ", "; }
        ss << std::to_string(shape[i]);
    }
    ss << "]";
    return ss.str();
}

// -
Array Array::elementwise_binary_broadcast(
    const Array& other, Array::BinaryFn&& fn
) const{
    // -
    const Shape out_shape = this->broadcast_shape(this->m_shape, other.m_shape);
    const Shape out_strides = this->broadcast_strides(out_shape, {});

    Array result(out_shape);

    // We'll iterate over the output linear index and map to both operands
    const std::size_t out_size = this->product(out_shape);

    // Parallel over output elements
    this->parallel_for(
        out_size,
        [this, &other, &result, fn, out_shape, out_strides](
            std::size_t begin, std::size_t end
        ){
            for(std::size_t idx=begin; idx < end; ++idx){
                // Convert linear index to multi-index in output
                Shape out_idx(out_shape.size());
                {
                    std::size_t temp = idx;
                    for(std::size_t i=out_shape.size(); i > 0; i--){
                        out_idx[i] = temp % out_shape[i];
                        temp /= out_shape[i];
                    }
                }

                // Map to a-indices and b-indices using broadcasting
                Shape a_idx(this->m_shape.size());
                Shape b_idx(other.m_shape.size());

                // Align trailing dimensions
                const std::size_t start_a = std::max(
                    0,
                    static_cast<int>(out_shape.size()) - static_cast<int>(this->m_shape.size())
                );
                const std::size_t start_b = std::max(
                    0,
                    static_cast<int>(out_shape.size()) - static_cast<int>(other.m_shape.size())
                );

                for(std::size_t i=0; i < out_shape.size(); ++i){
                    const std::size_t out_dim = out_shape[i];
                    const std::size_t out_stride = out_strides[i];

                    // Compute index in output
                    // (We already have out_idx[i])

                    // Map to a
                    if(i >= start_a){
                        const std::size_t a_dim_idx = i - start_a;
                        if(this->m_shape.size() == 0){
                            a_idx.clear();
                        }else{
                            a_idx[a_dim_idx] = out_idx[i] % (this->m_shape[a_dim_idx]);
                        }
                    }

                    // Map to b
                    if(i >= start_b){
                        const std::size_t b_dim_idx = i - start_b;
                        if(other.m_shape.size()==0){
                            b_idx.clear();
                        }else{
                            b_idx[b_dim_idx] = out_idx[i] % (other.m_shape[b_dim_idx]);
                        }
                    }
                }

                // Compute linear offsets
                std::size_t a_off = 0;
                for(std::size_t i=0; i < this->m_shape.size(); ++i){
                    a_off += a_idx[i] * this->m_strides[i];
                }

                std::size_t b_off = 0;
                for(std::size_t i=0; i < other.m_shape.size(); ++i){
                    b_off += b_idx[i] * other.m_strides[i];
                }

                std::size_t r_off = 0;
                for(std::size_t i=0; i < out_shape.size(); ++i){
                    r_off += out_idx[i] * out_strides[i];
                }

                result.m_data[r_off] = fn(this->m_data[a_off], other.m_data[b_off]);
            }
        }
    );

    return result;
}

// -
// 1D slice: [start, stop) with step
Array Array::slice(std::size_t start, std::size_t stop, std::size_t step) const{
    if(this->ndim() != 1){
        throw ParsevalError("`slice()`: expected array to be one-dimensional.");
    }

    if(step==0){
        throw ParsevalError("`slice()`: step cannot be zero");
    }

    const std::size_t n = this->m_shape[0];
    if(start >= n){
        return Array({0}); // empty array is returned
    }

    // Normalize
    stop = stop==0 ? n : stop;
    if(start > stop){
        return Array({0}); // empty array is returned
    }

    std::size_t count = (stop - start + step - 1)/ step;
    if(count==0){
        return Array({0});
    }

    if(count > n){
        count = (stop - start + step - 1) / step;
        if(count == 0){
            return Array({0});
        }
    }

    // Compute actual count
    for(std::size_t i=start; i < stop; i+= step){
        ++count;
    }
    Array result({count});
    std::size_t j=0;
    for(std::size_t i=start; i < stop && i < n; i+= step){
        result.m_data[j++] = this->m_data[i];
    }

    return result;
}

// -
// 2D slice: rows [r_start, r_stop), cols [c_start, c_stop) with step
Array Array::slice(
    std::size_t r_start, std::size_t r_stop,
    std::size_t c_start, std::size_t c_stop,
    std::size_t r_step, std::size_t c_step
) const{
    if(this->ndim() != 2){
        throw ParsevalError("`slice()`: expected array to be two-dimensional.");
    }

    if(r_step==0 || c_step==0){
        throw ParsevalError("`slice()`: step cannot be zero.");
    }

    const std::size_t rows = this->m_shape[0];
    const std::size_t cols = this->m_shape[1];

    if(r_start >= rows){
        return Array({0, 0});   // empty matrix is returned
    }

    if(c_start >= cols){
        return Array({0, 0});   // return an empty matrix
    }

    std::size_t r_count = 0;
    for(std::size_t i=r_start; i < r_stop && i < rows; i += r_step){
        ++r_count;
    }

    std::size_t c_count = 0;
    for(std::size_t i=c_start; i < c_stop && i < cols; i += c_step){
        ++c_count;
    }

    if(r_count==0 || c_count==0){
        return Array({0, 0});
    }

    Array result({r_count, c_count});
    std::size_t r = 0;
    for(std::size_t i=r_start; i < r_stop && i < rows; i += r_step){
        std::size_t c = 0;
        for(std::size_t j=c_start; j < c_stop && j < cols; j +=c_step){
            result(r, c++) = (*this)(i, j);
        }
        ++r;
    }

    return result;
}

// --------------------------------------
// --- Common Math function as method ---
// --------------------------------------
Array Array::cos(void) const{
    return this->apply_unary([](value_type x){ return std::cos(x); });
}

// -
Array Array::sin(void) const{
    return this->apply_unary([](value_type x){ return std::sin(x); });
}

// -
Array Array::tan(void) const{
    return this->apply_unary([](value_type x){ return std::tan(x); });
}

// -
Array Array::acos(void) const{
    return this->apply_unary([](value_type x){ return std::acos(x); });
}

// -
Array Array::asin(void) const{
    return this->apply_unary([](value_type x){ return std::asin(x); });
}

// 
Array Array::atan(void) const{
    return this->apply_unary([](value_type x){ return std::atan(x); });
}

// -
Array Array::cosh(void) const{
    return this->apply_unary([](value_type x){ return std::cosh(x); });
}

// -
Array Array::sinh(void) const{
    return this->apply_unary([](value_type x){ return std::sinh(x); });
}

// -
Array Array::tanh(void) const{
    return this->apply_unary([](value_type x){ return std::tanh(x); });
}

// -
Array Array::acosh(void) const{
    return this->apply_unary([](value_type x){ return std::acosh(x); });
}

// -
Array Array::asinh(void) const{
    return this->apply_unary([](value_type x){ return std::asinh(x); });
}

// -
Array Array::atanh(void) const{
    return this->apply_unary([](value_type x){ return std::atanh(x); });
}

// -
Array Array::exp(void) const{
    return this->apply_unary([](value_type x){ return std::exp(x); });
}

// -
Array Array::log(void) const{
    return this->apply_unary([](value_type x){
        if(x <= 0.0){
            throw ParsevalError("`log` requires positive values.");
        }
        return std::log(x);
    });
}

// -
Array Array::sqrt(void) const{
    return this->apply_unary([](value_type x){
        if(x < 0.0){
            throw ParsevalError("`sqrt` requires non-negative values");
        }
        return std::sqrt(x);
    });
}

// -
Array Array::abs(void) const{
    return this->apply_unary([](value_type x){ return std::abs(x); });
}

// -
Array Array::apply_unary(UnaryFn&& fn) const{
    Array result(this->m_shape);

    this->parallel_for(
        this->size(),
        [this, &result, fn](std::size_t begin, std::size_t end){
            for(std::size_t i=begin; i < end; ++i){
                result.m_data[i] = fn(this->m_data[i]);
            }
        }
    );

    return result;
}

// -----------------------------
// --- Functional primitives ---
// -----------------------------
Array Array::map(UnaryFn&& fn) const{
    return this->apply_unary(std::move(fn));
}

// -
Array Array::map(const Array& other, BinaryFn&& fn) const{
    return this->elementwise_binary_broadcast(other, std::move(fn));
}

// -
bool Array::any(UnaryPredicate&& predicate) const{
    bool result{false};
    for(const auto& x: this->m_data){
        if(predicate(x)){
            result = true;
            break;
        }
    }

    return result;
}

bool Array::all(UnaryPredicate&& predicate) const{
    bool result{true};
    for(const auto& x: this->m_data){
        if(!predicate(x)){
            result = false;
            break;
        }
    }

    return result;
}

// -
Array Array::reduce_axis(int axis, BinaryFn&& op) const{
    if(this->ndim() != 2){
        throw ParsevalError("reduce_axis requires a 2D array");
    }
    if(axis < -1 || axis > 0){
        throw ParsevalError("axis must be -1 or 0 for 2D");
    }
    axis = (axis == -1) ? 0 : axis;

    const std::size_t rows = this->m_shape[0];
    const std::size_t cols = this->m_shape[1];

    if(axis == 0){
        // Reduce rows -> 1D of length cols
        Array result({cols});
        for(std::size_t j=0; j < cols; ++j){
            value_type acc = 0.0;
            for(std::size_t i=0; i < rows; ++i){
                acc = op(acc, (*this)(i, j));
            }
            result(j) = acc;
        }
        return result;
    }else{
        // Reduce columns -> 1D of length rows
        Array result({rows});
        for(std::size_t i=0; i < rows; ++i){
            value_type acc = 0.0;
            for(std::size_t j=0; j < cols; ++j){
                acc = op(acc, (*this)(i, j));
            }
            result(i) = acc;
        }
        return result;
    }
}

// - Apply a function to each element; returns same shape
Array Array::apply(UnaryFn&& fn) const{
    return this->apply_unary(std::move(fn));
}


// -------------------
// -*- I/O Methods -*-
// -------------------
void Array::save_text(const std::string& path) const{
    std::ofstream fout(path);
    if(!fout){
        throw ParsevalError("Unable to open file for writing: " + path);
    }
    fout << "shape: ";
    for(std::size_t i=0; i < this->m_shape.size(); ++i){
        if(i > 0){ fout << ", "; }
        fout << this->m_shape[i];
    }
    fout << "\n";
    for(const auto& val: this->m_data){
        fout << val << "\n";
    }
    fout.close();
}

// -
Array Array::load_text(const std::string& path){
    std::ifstream fin(path);
    if(!fin){
        throw ParsevalError("Unable to open file for reading: " + path);
    }

    std::string line;
    if(!std::getline(fin, line)){
        fin.close();
        throw ParsevalError("Empty file");
    }

    // Parse shape
    std::vector<std::string> parts{};
    {
        std::istringstream stream(line);
        std::string token;
        std::getline(stream, token, ':');   // skip "shape"
        std::getline(stream, token);        // rest
        std::istringstream info(token);
        std::string text{};
        while(std::getline(info, text, ',')){
            // trim spaces
            std::string numstr;
            for(char c: text){
                if(!std::isspace(static_cast<unsigned char>(c))){
                    numstr += c;
                }
            }
            if(!numstr.empty()){
                parts.push_back(numstr);
            }
        }
    }

    Shape shape{};
    shape.reserve(parts.size());
    for(const auto& p: parts){
        shape.push_back(std::stoull(p));
    }

    std::vector<value_type> data;
    value_type val{};
    while(fin >> val){
        data.push_back(val);
    }

    if(Array::product(shape) != data.size()){
        fin.close();
        throw ParsevalError("Data size does not match shape");
    }

    fin.close();

    return Array(std::move(shape), std::move(data));
}

// - Binary I/O
// Format:
//  - 8-byte little-endian: number of dimensions
//  - for-each dim: 8-byte size
//  - the double values

// -
void Array::save_binary(const std::string& path) const{
    std::ofstream fout(path, std::ios::binary);
    if(!fout){
        throw ParsevalError("Unable to open binary file for writing: " + path);
    }

    std::size_t N = this->m_shape.size();
    fout.write(reinterpret_cast<char*>(&N), sizeof(N));
    for(std::size_t n: this->m_shape){
        fout.write(reinterpret_cast<char*>(&n), sizeof(n));
    }
    fout.write(
        reinterpret_cast<const char*>(this->m_data.data()),
        static_cast<std::streamsize>(this->m_data.size() * sizeof(value_type))
    );
    fout.close();
}

// -
Array Array::load_binary(const std::string& path){
    std::ifstream fin(path, std::ios::binary);
    if(!fin){
        throw ParsevalError("Unable to open binary file for reading: " + path);
    }

    std::size_t N{};
    fin.read(reinterpret_cast<char*>(&N), sizeof(N));

    Shape shape{};
    shape.reserve(N);
    for(std::size_t i=0; i < N; ++i){
        std::size_t n{};
        fin.read(reinterpret_cast<char*>(&n), sizeof(n));
        shape.push_back(n);
    }

    const std::size_t SIZE = Array::product(shape);
    std::vector<Array::value_type> data(SIZE);
    fin.read(
        reinterpret_cast<char*>(data.data()),
        static_cast<std::streamsize>(SIZE * sizeof(Array::value_type))
    );

    fin.close();

    return Array(std::move(shape), std::move(data));
}

// -
void Array::save_csv(const std::string& path, char delimiter) const{
    if(this->ndim() != 2){
        throw ParsevalError("`save_csv()` requires a 2D array");
    }

    std::ofstream fout(path);
    if(!fout){
        throw ParsevalError("Unable to open CSV file for writing: " + path);
    }

    const std::size_t rows = this->m_shape[0];
    const std::size_t cols = this->m_shape[1];

    for(std::size_t i=0; i < rows; ++i){
        for(std::size_t j=0; j < cols; ++j){
            if(j > 0){ fout << delimiter; }
            fout << (*this)(i, j);
        }
        fout << "\n";
    }
    fout.close();
}

// -
Array Array::load_csv(const std::string& path, char delimiter){
    std::ifstream fin(path);
    if(!fin){
        throw ParsevalError("Unable to open CSV file for reading: " + path);
    }

    std::vector<std::vector<Array::value_type>> rows;
    std::string line{};
    while(std::getline(fin, line)){
        if(line.empty()){ continue;}
        std::vector<Array::value_type> row;
        std::istringstream stream(line);
        std::string token;
        while(std::getline(stream, token, delimiter)){
            row.push_back(std::stod(token));
        }
        if(!row.empty()){
            rows.push_back(std::move(row));
        }
    }

    if(rows.empty()){
        fin.close();
        return Array({0, 0}); // empty matrix
    }

    const std::size_t nr = rows.size();
    const std::size_t nc = rows[0].size();
    if(nc==0){
        fin.close();
        return Array({nr, 0});
    }

    for(const auto& row: rows){
        if(row.size() != nc){
            fin.close();
            throw ParsevalError("Inconsistent CSV row lengths");
        }
    }

    std::vector<Array::value_type> data{};
    data.reserve(nr * nc);
    for(const auto& row: rows){
        data.insert(data.end(), row.begin(), row.end());
    }

    fin.close();
    return Array({nr, nc}, std::move(data));
}

// -
void Array::save_sqlite(
    const std::string& path,
    const std::string& table_prfix
) const{
    if(this->ndim() > 2){
        throw ParsevalError("`save_sqlite()` supports 1D or 2D arrays");
    }

    sqlite3* db = nullptr;
    auto status = sqlite3_open_v2(
        path.c_str(), &db,
        SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, nullptr
    );
    if(status != SQLITE_OK){
        throw ParsevalError(
            "SQLite open failed: " + std::string(sqlite3_errmsg(db))
        );
    }

    std::string query{};

    std::string table_name = table_prfix + "_array";

    query = "DROP TABLE IF EXIST " + table_name + ";";
    status = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
    if(status != SQLITE_OK){
        sqlite3_close(db);
        throw ParsevalError(
            "SQLite drop failed: "  + std::string(sqlite3_errmsg(db))
        );
    }

    if(this->ndim() == 1){
        query = "CREATE TABLE " + table_name +
            " (row INTEGER PRIMARY KEY, value REAL);" ;
    }else{
        query = "CREATE TABLE " + table_name +
            " (row INTEGER, col INTEGER, value REAL, PRIMARY KEY(row, col));";
    }

    status = sqlite3_exec(
        db, query.c_str(),
        nullptr, nullptr, nullptr
    );
    if(status != SQLITE_OK){
        sqlite3_close(db);
        throw ParsevalError(
            "SQLite create failed: " + std::string(sqlite3_errmsg(db))
        );
    }

    if(this->ndim() == 1){
        query = "INSERT INTO " + table_name + " (row, value) VALUES (?, ?);";
    }else{
        query = "INSERT INTO "  + table_name + " (row, col, value) VALUES (?, ?, ?);";
    }

    sqlite3_stmt* stmt;
    status = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    if(status != SQLITE_OK){
        sqlite3_close(db);
        std::stringstream ss;
        ss << "SQLite prepare failed: " << sqlite3_errmsg(db);
        throw ParsevalError(ss.str());
    }

    if(this->ndim() == 1){
        for(std::size_t i=0; i < this->size(); ++i){
            sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(i));
            sqlite3_bind_double(stmt, 2, this->m_data[i]);
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
        }
    }else{
        for(std::size_t i=0; i < this->m_shape[0]; ++i){
            for(std::size_t j=0; j < this->m_shape[1]; ++j){
                sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(i));
                sqlite3_bind_int64(stmt, 2, static_cast<sqlite3_int64>(j));
                sqlite3_bind_double(stmt, 3, (*this)(i, j));
                sqlite3_step(stmt);
                sqlite3_reset(stmt);
            }
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}


Array Array::load_sqlite(const std::string& path, const std::string& table_prefix){
    std::string table_name = table_prefix + "_array";

    sqlite3* db = nullptr;

    auto status = sqlite3_open_v2(
        path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr
    );
    if(status != SQLITE_OK){
        std::stringstream ss;
        ss << "SQLite open failed: " << sqlite3_errmsg(db);
        throw ParsevalError(ss.str());
    }

    std::string query = "SELECT row, col, value FROM " + table_name +
        " ORDER BY row, col;";
    
    sqlite3_stmt* stmt;
    status = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    if(status != SQLITE_OK){
        sqlite3_close(db);
        std::stringstream ss;
        ss << "SQLite prepare failed: " << sqlite3_errmsg(db);
        throw ParsevalError(ss.str());
    }

    std::vector<std::tuple<std::size_t, std::size_t, Array::value_type>> cells{};
    while(sqlite3_step(stmt) == SQLITE_ROW){
        auto r = static_cast<std::size_t>(sqlite3_column_int64(stmt, 0));
        auto c = static_cast<std::size_t>(sqlite3_column_int64(stmt, 1));
        auto val = static_cast<Array::value_type>(sqlite3_column_double(stmt,2));
        cells.emplace_back(r, c, val);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    if(cells.empty()){
        return Array({0, 0});
    }

    // Determine shape
    std::size_t nrow = 0;
    std::size_t ncol = 0;
    for(const auto& cell: cells){
        nrow = std::max(nrow, std::get<0>(cell));
        ncol = std::max(ncol, std::get<1>(cell));
    }

    // If ncol == 0, it's 1D
    auto is_1d = (ncol == 0);
    Shape shape{};
    if(is_1d){
        shape = {nrow+1};
    }else{
        shape = {nrow+1, ncol+1};
    }

    Array result(shape);
    for(const auto& cell: cells){
        std::size_t r = std::get<0>(cell);
        std::size_t c = std::get<1>(cell);
        Array::value_type val = std::get<2>(cell);
        if(is_1d){
            result(r) = val;
        }else{
            result(r, c) = val;
        }
    }

    return result;
}

// -
void Array::set_attribute(const std::string& name, const std::string& value){
    this->m_attributes[name] = value;
}

// -
std::string Array::get_attribute(const std::string& name) const{
    auto entry = this->m_attributes.find(name);
    if(entry != this->m_attributes.end()){
        return entry->second;
    }

    return "";  // or throw an exception
}

// -
bool Array::has_attribute(const std::string& name) const{
    return this->m_attributes.find(name) != this->m_attributes.end();
}

const std::map<std::string, std::string>& Array::attributes(void) const{}


/*
class Array{
public:
    using value_type = double;
    using Shape = std::vector<std::size_t>;


    static std::vector<std::size_t> to_netcdf_shape(const Shape& shape){
        return std::vector<std::size_t>(shape.begin(), shape.end());
    }

void Array::save_netcdf(
    const std::string& path,
    const std::string& var_name="data",
    int compression_level=0,            // 0 = no compression, 9 = max
    bool shuffle=true                   // Shuffle filter (improve compression)
) const;
Array Array::load_netcdf(const std::string& path, const std::string& var_name="data");

void Array::save_netcdf_multiple(
    const std::string& path,
    const std::map<std::string, Array>& variables,
    int compression_level=0,
    bool shuffle=true
) const;

std::map<std::string, Array> Array::load_netcdf_multiple(const std::string& path);



private:
    Shape m_shape;
    Shape m_strides;
    std::vector<value_type> m_data;
    std::map<std::string, std::string> m_attributes; // Key-value pairs for metadata

    static constexpr std::size_t parallel_threshold = 4096;


};

*/


// --------------------------------------------------------------------
}//-*- end::namespace::parseval                                     -*-
// --------------------------------------------------------------------
