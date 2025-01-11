#pragma once

#include "core.hpp"
#include <cstdio>
#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <cassert>
#include <type_traits>
#include <unordered_map>

namespace srpc {

enum rpc_status_code : uint8_t {
	RPC_SUCCESS = 0,
	RPC_ERR_FUNCTION_NOT_REGISTERED,
	RPC_ERR_RECV_TIMEOUT
};

template <typename T>
class request_t {
public:
    T value() const { return _value; }
    const std::string& method_name() const { return _method_name; }
    
    void set_value(T&& v) { _value = std::move(v); }
    void set_method_name(std::string const& s) { _method_name = s; }
    
private:
    std::string _method_name;
    T           _value;
};

template <typename T>
class response_t {
public:
    response_t() : _code(RPC_SUCCESS) {};
    ~response_t() {};

    rpc_status_code code() const { return _code; }
    T value() const { return _value; }

    void set_code(rpc_status_code c) { _code = c; }
    void set_value(T const& v) { _value = v; }

private:
    rpc_status_code _code;
    T               _value;
}; 

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

class packer {
public:
    using ptr = std::shared_ptr<packer>;

    packer() { _buf = std::make_shared<buffer>(); }
    packer(std::vector<uint8_t> const& bytes) { _buf = std::make_shared<buffer>(bytes); }
    packer(std::vector<uint8_t>&& bytes) { _buf = std::make_shared<buffer>(std::move(bytes)); }
    packer(buffer::ptr buf_ptr) : _buf(buf_ptr) {}

    constexpr const uint8_t* data() { return _buf->curdata(); }
    constexpr size_t size() { return _buf->cursize(); }
    constexpr size_t offset() const noexcept { return _buf->offset(); }
    constexpr void clear() noexcept { _buf->reset(); }
    buffer::ptr buf() noexcept { return _buf; }
   
    template <typename T>
    constexpr packer& operator>>(T& v) { pipe_output(v); return *this; };

    template <typename T>
    constexpr packer& operator<<(T v) { pack_arg(v); return *this; };

    /// To pack bytes with the method_name, service_name and message_name as header. 
    /// Used to pack the outermost struct from client to server (a client request).
    template <typename T> requires (has_name_v<T> && has_fields_v<T>)
    constexpr void pack_request(request_t<T> const& req) {
        pack_arg(req.method_name());
        pack_arg(T::name);
        pack_struct(req.value());
    }

    /// To pack bytes with the message's name as the header. 
    /// Used to pack the outermost struct from server to client (a server response).
    template <typename T> requires (has_name_v<T> && has_fields_v<T>)
    constexpr void pack_response(response_t<T> const& resp) {
        pack_arg(resp.code());
        pack_arg(T::name);
        pack_struct(resp.value());
    }    
     
    /// To be called at the server, unpacks a client request. 
    /// @tparam R request struct type
    template <typename R>
    [[nodiscard]] request_t<R> unpack_request() noexcept { 
        request_t<R> req;

        std::string method_name;
        *this >> method_name;
        req.set_method_name(method_name);

        std::string message_name;
        *this >> message_name;
     
        auto it = message_registry.find(message_name);
        // TODO: change this to 'it == message_registry.end()' and add error exception 
        // (package and send out some error in the request_t class)
        if (it != message_registry.end()) {
            std::unique_ptr<R> msg_ptr(dynamic_cast<R*>(it->second().release()));
            msg_ptr->unpack(_buf);
            req.set_value(std::move(*msg_ptr));
        }

        return req;
    }

    /// To be called at the client side, unpacks a server response.
    /// @tparam R response struct type
    template <typename R>
    [[nodiscard]] response_t<R> unpack_response() noexcept {
        response_t<R> res;
        
        // read status code
        rpc_status_code status;
        *this >> status;
        res.set_code(status);

        // read message name
        std::string message_name;
        *this >> message_name;
        
        auto it = message_registry.find(message_name);
        // TODO: change this to 'it == message_registry.end()' and add error exception 
        // (package and send out some error in the response_t class)
        if (it != message_registry.end()) { 
            std::unique_ptr<R> msg_ptr(dynamic_cast<R*>(it->second().release()));
            msg_ptr->unpack(_buf);
            res.set_value(std::move(*msg_ptr));
        }

        return res;
    }
    
    /// To be called to get the message in a buffer without metadata
    template <typename T>
    [[nodiscard]] T* getv() noexcept {
        std::string message_name;
        *this >> message_name;

        auto it = message_registry.find(message_name);
        if (it == message_registry.end()) { 
            fprintf(stderr, "message %s not found!", message_name.c_str());
            return nullptr; 
        }
        T* v(dynamic_cast<T*>(it->second().release()));
        v->unpack(_buf);

        assert(size() == 0);
     
        return v;
    }

private:
    template <typename T>
    constexpr void pipe_output(T& v) noexcept;
    
    template <>
    void pipe_output(std::string& v) noexcept; 

    template <typename T>
    constexpr void pack_arg(T const& arg) noexcept;
    
    /// Packs message structs by using the T::fields tuple the message comes with
    template <typename T> requires has_fields_v<T>
    constexpr void pack_struct(T const& arg) noexcept {
        std::apply(
            [this, &arg] (auto... member) { 
                (pack_arg(arg.*(decltype(member)::member_ptr)), ...); 
            },
            T::fields
        );
    }

    buffer::ptr _buf;
};

template <typename T>
constexpr void packer::pack_arg(T const& arg) noexcept {
    if constexpr (std::is_base_of_v<message_base, T>) {
        pack_struct(arg);
    } else {
        const uint8_t* data = reinterpret_cast<const uint8_t*>(&arg);
        _buf->append(data, sizeof(T));
    }
}

template <>
inline void packer::pack_arg<std::string>(std::string const& arg) noexcept {
    size_t length = arg.size();
    pack_arg(length);
    _buf->append(arg.begin(), arg.end());
}

/// const char* const& might be a bit confusing. essentially its a const reference (can't reassign pointer)
/// to a const char pointer (pointer to a const char) meaning you can't modify the object it is pointing to.
/// so you cant modify nor can you modify the pointer to point to something else.
template <>
inline void packer::pack_arg<const char*>(const char* const& arg) noexcept {
    size_t length = std::strlen(arg);
    pack_arg(length);
    _buf->append(reinterpret_cast<const uint8_t*>(arg), std::strlen(arg));
}

template <typename T>
constexpr void packer::pipe_output(T& v) noexcept {
    std::memcpy(&v, _buf->curdata(), sizeof(T)); 
    _buf->increment(sizeof(T)); 
}

template <>
inline void packer::pipe_output(std::string& v) noexcept {
    int64_t strlen = 0;
    pipe_output(strlen);
    v = std::string(reinterpret_cast<const char*>(_buf->curdata()), strlen);
    _buf->increment(strlen); 
}

} // namespace srpc





