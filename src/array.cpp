#include "parseval/parseval.hpp"

#include<algorithm>
#include<numeric>
#include<utility>
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

/*
class Array{
public:
    using value_type = double;
    using Shape = std::vector<std::size_t>;

// Contructors
Array::Array(const Shape& shape, value_type value=0.0);
Array::Array(std::initializer_list<std::size_t> shape, value_type value=0.0);
Array::Array(const Shape& shape, const std::vector<value_type>& data);
Array::Array(const Shape& shape, std::vector<value_type>&& data);

// - static methods return Array
Array Array::zeros(const Shape& shape);
Array Array::ones(const Shape& shape);
Array Array::identity(size_t n);
Array Array::full(const Shape& shape, value_type value);
Array Array::arange(value_type start, value_type stop, value_type step=1.0);

std::size_t Array::ndim(void) const noexcept;
const Shape& Array::shape(void) const noexcept;
const Shape& Array::stride(void) const noexcept;
std::size_t Array::size(void) const noexcept;
bool Array::empty(void) const noexcept;
const std::vector<value_type>& Array::data(void) const noexcept;
std::vector<value_type>& Array::data(void) noexcept;
const value_type* Array::raw_data(void) const noexcept;
value_type* Array::raw_data(void) noexcept;
value_type& Array::at(const Shape& indices);
const value_type& Array::at(const Shape& indices) const;

template<typename... Indices>
value_type& operator()(Indices... indices);

template<typename... Indices>
const value_type& Array::operator()(Indices... indices) const;

Array Array::reshape(const Shape& myNewShape) const;
Array Array::transpose(void) const;
Array Array::matmul(const Array& other) const;

value_type Array::sum(void) const;
value_type Array::min(void) const;
value_type Array::max(void) const;

Array Array::operator+(const Array& other) const;
Array Array::operator-(const Array& other) const;
Array Array::operator*(const Array& other) const;
Array Array::operator/(const Array& other) const;

Array Array::operator+(value_type scalar) const;
Array Array::operator-(value_type scalar) const;
Array Array::operator*(value_type scalar) const;
Array Array::operator/(value_type scalar) const;

Array& Array::operator+=(const Array& other);
Array& Array::operator-=(const Array& other);
Array& Array::operator*=(const Array& other);
Array& Array::operator/=(const Array& other);

Array& Array::operator+=(value_type scalar);
Array& Array::operator-=(value_type scalar);
Array& Array::operator*=(value_type scalar);
Array& Array::operator/=(value_type scalar);

bool Array::is_close(const Array& other, value_type tolerance=1e-12) const;

// friends
Array operator+(value_type scalar, const Array& rhs);
Array operator-(value_type scalar, const Array& rhs);
Array operator*(value_type scalar, const Array& rhs);
Array operator/(value_type scalar, const Array& rhs);

std::ostream& operator<<(std::ostream& stream, const Array& array);

// -
void Array::svd(Array& u, Array& singular_values, Array& vt) const;
void Array::qr(Array& q, Array& r) const;
void Array::lu(Array& l, Array& u) const; //
void Array::cholesky(void) const;
void Array::eigen(Array& eigenValues, Array& eigenVectors) const;

Array Array::diag(void) const;
std::size_t Array::rank(double tolerance=1e-12) const;
value_type Array::det(void) const;
Array Array::inverse(void) const;
value_type Array::trace(void) const;

// 1D slice: [start, stop) with step
Array Array::slice(std::size_t start, std::size_t stop, std::size_t step=1) const;

// 2D slice: rows [r_start, r_stop), cols [c_start, c_stop) with step
Array Array::slice(
    std::size_t r_start, std::size_t r_stop,
    std::size_t c_start, std::size_t c_stop,
    std::size_t r_step=1, std::size_t c_step=1
) const;


// - Math function as method
Array Array::cos(void) const;
Array Array::sin(void) const;
Array Array::tan(void) const;
Array Array::acos(void) const;
Array Array::asin(void) const;
Array Array::atan(void) const;
Array Array::cosh(void) const;
Array Array::sinh(void) const;
Array Array::tanh(void) const;
Array Array::acosh(void) const;
Array Array::asinh(void) const;
Array Array::atanh(void) const;
Array Array::exp(void) const;
Array Array::log(void) const;
Array Array::sqrt(void) const;
Array Array::abs(void) const;


// Functional primitives
Array Array::map(auto&& fn) const;
Array Array::map(const Array& other, auto&& fn) const;
Array Array::any(int axis=-1) const;
Array Array::all(int axis=-1) const;

// - Apply a function to each element; returns same shape
Array Array::apply(auto&& fn) const;


// -----------
// -*- I/O -*-
// -----------
void Array::save_text(const std::string& path) const;
Array Array::load_text(const std::string& path);
void Array::save_binary(const std::string& path) const;
Array Array::load_binary(const std::string& path);
void Array::save_csv(const std::string& path, char delimiter=',') const;
Array Array::load_csv(const std::string& path, char delimiter=',');
void Array::save_sqlite(const std::string& path, const std::string& table_prfix) const;
Array Array::load_sqlite(const std::string& path, const std::string& table_prefix);

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

void Array::set_attribute(const std::string& name, const std::string& value);
std::string Array::get_attribue(const std::string& name) const;
bool Array::has_attribute(const std::string& name) const;
const std::map<std::string, std::string>& Array::attributes(void) const;


private:
    Shape m_shape;
    Shape m_strides;
    std::vector<value_type> m_data;
    std::map<std::string, std::string> m_attributes; // Key-value pairs for metadata


void Array::compute_strides(void);
std::size_t Array::offset(const Shape& indices) const;
void Array::validate_same_shape(const Array& other) const;


    static constexpr std::size_t parallel_threshold = 4096;

template<typename Function>
void Array::parallel_for(
    std::size_t count,
    Function&& function,
    std::size_t minimum_parallel_work=parallel_threshold
) const;

template<typename Operation>
Array Array::elementwise_binary(const Array& other, Operation&& operation) const;

template<typename Operation>
Array Array::elementwise_scalar(value_type scalar, Operation&& operation) const;

// utility method for handling Eigen library
void Array::require_matrix(const char* operation) const;
void Array::require_square_matrix(const char* operation) const;
Eigen::MatrixXd Array::to_eigen_matrix(void) const;
Array Array::from_eigen_matrix(const Eigen::MatrixXd& matrix);
Array Array::from_eigen_vector(const Eigen::VectorXd& vec);

Shape Array::broadcast_shape(const Shape& a, const Shape& b);
Shape Array::broadcast_strides(const Shape& shape, const Shape& strides);
std::string Array::to_string(const Shape& shape);

template<typename Operation>
Array Array::elementwise_binary_broadcast(
    const Array& other, Operation&& op
) const;

template<typename UnaryOp>
Array Array::apply_unary(UnaryOp&& op) const;

template<typename BinarOp>
Array Array::elementwise_binary_broad_cast(
    const Array& other, BinarOp op
) const;

Array Array::reduce_axis(int axis, auto&& op) const;
};

*/


// --------------------------------------------------------------------
}//-*- end::namespace::parseval                                     -*-
// --------------------------------------------------------------------
