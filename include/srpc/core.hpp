#pragma once

#include <stdexcept>
#include <functional>

namespace srpc {

#ifndef STRUCT_MEMBER
#define STRUCT_MEMBER(struct_t, member_name, member_str) std::make_tuple(member_str, &struct_t::member_name)
#endif

constexpr int MEMBER_NAME = 0;
constexpr int MEMBER_ADDR = 1;

struct buffer : public std::vector<uint8_t> {
    using ptr = std::shared_ptr<buffer>;

    constexpr buffer() : _offset(0) {}
    constexpr buffer(const uint8_t* bytes, size_t len) : _offset(0), std::vector<uint8_t>(bytes, bytes + len) {} 
    constexpr buffer(std::vector<uint8_t> const& bytes) : _offset(0), std::vector<uint8_t>(bytes) {} 
    constexpr buffer(std::vector<uint8_t>&& bytes) : _offset(0), std::vector<uint8_t>(std::move(bytes)) {} 
    
    constexpr size_t cursize() noexcept { return size() - _offset; }
    constexpr size_t offset() noexcept { return _offset; }
    constexpr const uint8_t* data() noexcept { return &(*this)[0]; }
    constexpr const uint8_t* curdata() noexcept { return &(*this)[_offset]; }
    void increment(int32_t k) {
        if (_offset + k > size()) {
            throw std::runtime_error("offset out of bounds!");
        }
        _offset += k; 
    }
    constexpr void append(const uint8_t* s, size_t len) { insert(end(), s, s + len); }
    template <typename It> constexpr void append(It b, It e) { insert(end(), b, e); }
    constexpr void reset() { _offset = 0; clear(); }

private:
    size_t _offset;
};

/// To be inherited by generated messages
struct message_base {
    virtual ~message_base() = default;
    virtual void unpack(buffer::ptr bp) {};

    static constexpr const char* name = nullptr;
    static constexpr auto fields = std::make_tuple();
};

/// To be inherited by generated messages
struct servicer_base {
    virtual ~servicer_base() = default;

    static constexpr const char* name = nullptr;
    static constexpr auto methods = std::make_tuple();
};

template <typename F>
struct function_traits;

/// @tparam I input type of function
/// @tparam R return type of function
template <typename R, typename I>
struct function_traits<R (*)(const I&)> {
    using input_type = I;
    using return_type = R;
};

template <typename I, typename R>
struct function_traits<std::function<R(const I&)>> {
    using input_type = I;
    using return_type = R;
};

/// Member function specialization
/// @tparam C class of member function
/// @tparam I input type of function
/// @tparam R return type of function
template <typename C, typename R, typename I>
struct function_traits<R (C::*)(I)>{
    using class_type = C;
    using input_type = I;
    using return_type = R;
};

/// Const member function specialization
template <typename C, typename R, typename I>
struct function_traits<R (C::*)(I...) const> {
    using class_type = C;
    using input_type = I;
    using return_type = R;
};

template <typename T, typename = void>
struct has_methods : std::false_type {};

template <typename T>
struct has_methods<T, std::void_t<decltype(T::methods)>> : std::true_type {};

template <typename T>
constexpr bool has_methods_v = has_methods<T>::value;

template <typename T, typename = void>
struct has_fields : std::false_type {};

template <typename T>
struct has_fields<T, std::void_t<decltype(T::fields)>> : std::true_type {};

template <typename T>
constexpr bool has_fields_v = has_fields<T>::value;

template <typename T, typename = void>
struct has_name : std::false_type {};

template <typename T>
struct has_name<T, std::void_t<decltype(T::name)>> : std::true_type {};

template <typename T>
constexpr bool has_name_v = has_name<T>::value;

template <typename D, typename B>
concept Derived = std::is_base_of_v<B, D>;

template <typename T>
concept SrpcMessage = has_name_v<T> && has_fields_v<T> && Derived<T, message_base>;

template <typename T>
concept SrpcService = has_name_v<T> && has_methods_v<T> && Derived<T, servicer_base>;

using message_factory = std::function<std::unique_ptr<message_base>()>;

/// To be populated with user generated structs 
static std::unordered_map<std::string, message_factory> message_registry {};

} // namespace srpc

