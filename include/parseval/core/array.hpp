#pragma once

#include<map>
#include<vector>
#include<string>
#include<stdexcept>
#include<iostream>
#include<initializer_list>

#include<Eigen/Core>

#include "thread_pool.hpp"


// --------------------------------------------------------------------
// -*- begin::namespace::parseval                                   -*-
// --------------------------------------------------------------------
namespace parseval{
// -

class ParsevalError : public std::runtime_error{
public:
    ParsevalError()
    : std::runtime_error("ParsevalError: unknown error caugth")
    {}

    explicit ParsevalError(const char* msg)
    : std::runtime_error(std::string("ParsevalError: ") + msg)
    {}

    explicit ParsevalError(const std::string& msg)
    : ParsevalError(msg.c_str())
    {}
};

class Array{
public:
    using value_type = double;
    using Shape = std::vector<std::size_t>;

    // Contructors
    Array() = default;
    explicit Array(const Shape& shape, value_type value=0.0);
    Array(std::initializer_list<std::size_t> shape, value_type value=0.0);
    Array(const Shape& shape, const std::vector<value_type>& data);
    Array(const Shape& shape, std::vector<value_type>&& data);

    // static methods return Array
    static Array zeros(const Shape& shape);
    static Array ones(const Shape& shape);
    static Array identity(size_t n);
    static Array full(const Shape& shape, value_type value);
    static Array arange(
        value_type start,
        value_type stop,
        value_type step=1.0
    );

    std::size_t ndim(void) const noexcept;
    const Shape& shape(void) const noexcept;
    const Shape& stride(void) const noexcept;
    std::size_t size(void) const noexcept;
    bool empty(void) const noexcept;
    const std::vector<value_type>& data(void) const noexcept;
    std::vector<value_type>& data(void) noexcept;
    const value_type* raw_data(void) const noexcept;
    value_type* raw_data(void) noexcept;
    value_type& at(const Shape& indices);
    const value_type& at(const Shape& indices) const;

    template<typename... Indices>
    value_type& operator()(Indices... indices);

    template<typename... Indices>
    const value_type& operator()(Indices... indices) const;

    Array reshape(const Shape& myNewShape) const;
    Array transpose(void) const;
    Array matmul(const Array& other) const;

    value_type sum(void) const;
    value_type min(void) const;
    value_type max(void) const;

    Array operator+(const Array& other) const;
    Array operator-(const Array& other) const;
    Array operator*(const Array& other) const;
    Array operator/(const Array& other) const;

    Array operator+(value_type scalar) const;
    Array operator-(value_type scalar) const;
    Array operator*(value_type scalar) const;
    Array operator/(value_type scalar) const;

    Array& operator+=(const Array& other);
    Array& operator-=(const Array& other);
    Array& operator*=(const Array& other);
    Array& operator/=(const Array& other);

    Array& operator+=(value_type scalar);
    Array& operator-=(value_type scalar);
    Array& operator*=(value_type scalar);
    Array& operator/=(value_type scalar);

    bool is_close(const Array& other, value_type tolerance=1e-12) const;

    friend Array operator+(value_type scalar, const Array& rhs);
    friend Array operator-(value_type scalar, const Array& rhs);
    friend Array operator*(value_type scalar, const Array& rhs);
    friend Array operator/(value_type scalar, const Array& rhs);

    friend std::ostream& operator<<(std::ostream& stream, const Array& array);

    void svd(Array& u, Array& singular_values, Array& vt) const;
    void qr(Array& q, Array& r) const;
    void lu(Array& l, Array& u, Array& p) const; //
    Array cholesky(void) const;
    void eigen(Array& eigenValues, Array& eigenVectors) const;

    Array diag(void) const;
    std::size_t rank(void) const;
    value_type det(void) const;
    Array inverse(void) const;
    value_type trace(void) const;

    // 1D slice: [start, stop) with step
    Array slice(std::size_t start, std::size_t stop, std::size_t step=1) const;

    // 2D slice: rows [r_start, r_stop), cols [c_start, c_stop) with step
    Array slice(
        std::size_t r_start,
        std::size_t r_stop,
        std::size_t c_start,
        std::size_t c_stop,
        std::size_t r_step=1,
        std::size_t c_step=1
    ) const;


    // Math function as method
    Array cos(void) const;
    Array sin(void) const;
    Array tan(void) const;
    Array acos(void) const;
    Array asin(void) const;
    Array atan(void) const;
    Array cosh(void) const;
    Array sinh(void) const;
    Array tanh(void) const;
    Array acosh(void) const;
    Array asinh(void) const;
    Array atanh(void) const;
    Array exp(void) const;
    Array log(void) const;
    Array sqrt(void) const;
    Array abs(void) const;


    Array map(auto&& fn) const;
    Array map(const Array& other, auto&& fn) const;
    Array any(int axis=-1) const;
    Array all(int axis=-1) const;
    // Apply a function to each element; returns same shape
    Array apply(auto&& fn) const;


    // -----------
    // -*- I/O -*-
    // -----------
    void save_text(const std::string& path) const;
    static Array load_text(const std::string& path);
    void save_binary(const std::string& path) const;
    static Array load_binary(const std::string& path);
    void save_csv(const std::string& path, char delimiter=',') const;
    static Array load_csv(const std::string& path, char delimiter=',');
    void save_sqlite(const std::string& path, const std::string& table_prfix) const;
    static Array load_sqlite(const std::string& path, const std::string& table_prefix);

    static std::vector<std::size_t> to_netcdf_shape(const Shape& shape){
        return std::vector<std::size_t>(shape.begin(), shape.end());
    }

    void save_netcdf(
        const std::string& path,
        const std::string& var_name="data",
        int compression_level=0,            // 0 = no compression, 9 = max
        bool shuffle=true                   // Shuffle filter (improve compression)
    ) const;
    static Array load_netcdf(const std::string& path, const std::string& var_name="data");

    void save_netcdf_multiple(
        const std::string& path,
        const std::map<std::string, Array>& variables,
        int compression_level=0,
        bool shuffle=true
    ) const;
    static std::map<std::string, Array> load_netcdf_multiple(
        const std::string& path
    );

    void set_attribute(const std::string& name, const std::string& value);
    std::string get_attribue(const std::string& name) const;
    bool has_attribute(const std::string& name) const;
    const std::map<std::string, std::string>& attributes(void) const;


private:
    Shape m_shape;
    Shape m_strides;
    std::vector<value_type> m_data;
    std::map<std::string, std::string> m_attributes; // Key-value pairs for metadata

    static std::size_t product(const Shape& shape);
    void compute_strides(void);
    std::size_t offset(const Shape& indices) const;
    void validate_same_shape(const Array& other) const;


    static constexpr std::size_t parallel_threshold = 4096;

    template<typename Function>
    void parallel_for(
        std::size_t count,
        Function&& function,
        std::size_t minimum_parallel_work=parallel_threshold
    ) const;

    template<typename BinaryOp>
    Array elementwise_binary(const Array& other, BinaryOp&& func) const;

    template<typename ScalarOp>
    Array elementwise_scalar(value_type scalar, ScalarOp&& func) const;

    // utility method for handling Eigen library
    void require_matrix(const char* operation) const;
    void require_square_matrix(const char* operation) const;
    Eigen::MatrixXd to_eigen_matrix(void) const;
    static Array from_eigen_matrix(const Eigen::MatrixXd& matrix);
    static Array from_eigen_vector(const Eigen::VectorXd& vec);

    static Shape broadcast_shape(const Shape& a, const Shape& b);
    static Shape broadcast_strides(const Shape& shape, const Shape& strides);
    static std::string to_string(const Shape& shape);

    template<typename Operation>
    Array elementwise_binary_broadcast(
        const Array& other, Operation&& op
    ) const;

    template<typename UnaryOp>
    Array apply_unary(UnaryOp&& op) const;

    template<typename BinarOp>
    Array elementwise_binary_broad_cast(
        const Array& other, BinarOp op
    ) const;

    Array reduce_axis(int axis, auto&& op) const;

};

Array operator+(Array::value_type scalar, const Array& rhs);
Array operator-(Array::value_type scalar, const Array& rhs);
Array operator*(Array::value_type scalar, const Array& rhs);
Array operator/(Array::value_type scalar, const Array& rhs);


// --------------------------------------------------------------------
}//-*- end::namespace::parseval                                     -*-
// --------------------------------------------------------------------


/*

#pragma once

// --------------------------------------------------------------------
// -*- begin::namespace::parseval                                   -*-
// --------------------------------------------------------------------
namespace parseval{
// -


// --------------------------------------------------------------------
}//-*- end::namespace::parseval                                     -*-
// --------------------------------------------------------------------

*/