#pragma once

#include <tuple>
#include <functional>
#include <unordered_map>

namespace srpc {

#ifndef MESSAGE_FIELD 
#define MESSAGE_FIELD(gen_struct, field) &gen_struct::field
#endif
    
#ifndef SERVICE_METHOD
#define SERVICE_METHOD(gen_struct, field) std::make_tuple(#field, &gen_struct::field)
#endif

// To be inherited by generated messages
struct message_base {
    virtual ~message_base() = default;
    virtual void unpack(const std::vector<uint8_t>& packed, size_t& offset) {};

    static constexpr const char* name = nullptr;
    static constexpr auto fields = std::make_tuple();
};

// To be inherited by generated services 
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

using message_factory = std::function<std::unique_ptr<message_base>()>;

// to be populated with user generated structs 
static std::unordered_map<std::string, message_factory> message_registry {};

} // namespace srpc

