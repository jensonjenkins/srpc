#pragma once

#include "core.hpp"
#include <cstdint>
#include <type_traits>
#include <vector>
#include <string>

namespace srpc {

enum rpc_status_code : uint8_t {
	RPC_SUCCESS = 0,
	RPC_ERR_FUNCTION_NOT_REGISTERRED,
	RPC_ERR_RECV_TIMEOUT
};

template <typename T>
class request_t {
public:
    T value() const { return _value; }
    std::string& method_name() { return _method_name; }
    
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

struct buffer {
    buffer() : _offset(0) {}
    
    const char* data() { return _data.data(); }
    
    void reset() { 
        _offset = 0;
        _data.clear();
    }

private:
    size_t              _offset;
    std::vector<char>   _data;
};

struct packer {
    template <typename T>
    constexpr static void pack_arg(std::vector<uint8_t>& buffer, T const& arg) noexcept {
        if constexpr (std::is_base_of_v<message_base, T>) {
            std::vector<uint8_t> nested_buffer = pack_nested(arg);
            buffer.insert(buffer.end(), nested_buffer.begin(), nested_buffer.end()); // WARN: copies the nested_buffer
                                                                                     // TODO: probably avoid this later
        } else {
            const uint8_t* data = reinterpret_cast<const uint8_t*>(&arg);
            buffer.insert(buffer.end(), data, data + sizeof(T));
        }
    }

    template <>
    void pack_arg<std::string>(std::vector<uint8_t>& buffer, std::string const& arg) noexcept {
        size_t length = arg.size();
        const uint8_t* start_addr = reinterpret_cast<const uint8_t*>(&length);
        buffer.insert(buffer.end(), start_addr, start_addr + sizeof(length));
        buffer.insert(buffer.end(), arg.begin(), arg.end());
    }

    /// const char* const& might be a bit confusing. essentially its a const reference (can't reassign pointer)
    /// to a const char pointer (pointer to a const char) meaning you can't modify the object it is pointing to.
    /// so you cant modify nor can you modify the pointer to point to something else.
    template <>
    void pack_arg<const char*>(std::vector<uint8_t>& buffer, const char* const& arg) noexcept {
        size_t length = std::strlen(arg);
        pack_arg(buffer, length);
        for (const char* ptr = arg; *ptr != '\0'; ++ptr) {
            buffer.push_back(static_cast<uint8_t>(*ptr));
        }
    }

    template <typename T>
    constexpr static void pack_tuple(std::vector<uint8_t>& buffer, T const& arg) noexcept {
        std::apply(
            [&buffer, &arg] (const auto&... field) {
                (pack_arg(buffer, arg.*(field)), ...); // get and pack message field 
            },
            T::fields
        );
    }

    /// To pack bytes with the method_name, service_name and message_name as header. 
    /// Used to pack the outermost struct from client to server (a client request).
    template <typename T> 
    [[nodiscard]] constexpr static std::vector<uint8_t> pack_request(const std::string method_name, T const& arg) {
        std::vector<uint8_t> buffer;
        pack_arg(buffer, method_name);
        pack_arg(buffer, T::name);
        pack_tuple(buffer, arg);
        return buffer;
    }

    /// To pack bytes with the message's name as the header. 
    /// Used to pack the outermost struct from server to client (a server response).
    template <typename T> 
    [[nodiscard]] constexpr static std::vector<uint8_t> pack_response(T const& arg) {
        std::vector<uint8_t> buffer;
        pack_arg(buffer, T::name);
        pack_tuple(buffer, arg);
        return buffer;
    }    
    
    /// To pack bytes without the message and method name. Used to pack nested structs.
    ///
    /// unimportant sidenote: The reason for this very similar method is to avoid a bool flag in pack()'s argument
    /// to not include the pack_arg(buffer, T::name) line. I personally think the method signature becomes ugly 
    /// with a bool flag to accomplish a trivial task. 
    template <typename T> 
    [[nodiscard]] constexpr static std::vector<uint8_t> pack_nested(T const& arg) {
        std::vector<uint8_t> buffer; 
        pack_tuple(buffer, arg);
        return buffer;
    }

    [[nodiscard]] static std::string read_string(const std::vector<uint8_t>& packed, size_t& offset) noexcept {
        int64_t header_length = 0;
        std::memcpy(&header_length, packed.data() + offset, sizeof(int64_t)); 
        offset += sizeof(int64_t);
        std::string result(reinterpret_cast<const char*>(packed.data() + offset), header_length);
        offset += header_length;
        
        return result;
    }
 
    /// To be called at the server, unpacks a client request. 
    /// @tparam R request struct type
    template <typename R>
    [[nodiscard]] static request_t<R> unpack_request(const std::vector<uint8_t>& packed) noexcept { 
        size_t offset = 0;
        request_t<R> req;

        // read method name
        req.set_method_name(read_string(packed, offset));
        // read message name
        std::string message_name = read_string(packed, offset);
     
        auto it = message_registry.find(message_name);
        if (it != message_registry.end()) {
            std::unique_ptr<R> msg_ptr(dynamic_cast<R*>(it->second().release()));
            msg_ptr->unpack(packed, offset); 
            req.set_value(std::move(*msg_ptr));
        }

        return req;
    }

    /// To be called at the client side, unpacks a server response.
    /// @tparam R response struct type
    template <typename R>
    [[nodiscard]] static response_t<R> unpack_response(const std::vector<uint8_t>& packed) noexcept {
        size_t offset = 0;
        rpc_status_code status;
        response_t<R> res;
        
        // read status code
        std::memcpy(&status, packed.data() + offset, sizeof(rpc_status_code)); 
        offset += sizeof(int8_t);
        res.set_code(status);
        // read message name
        std::string message_name = read_string(packed, offset);
        
        auto it = message_registry.find(message_name);
        if (it != message_registry.end()) {
            std::unique_ptr<R> msg_ptr(dynamic_cast<R*>(it->second().release()));
            msg_ptr->unpack(packed, offset); 
            res.set_value(std::move(*msg_ptr));
        }

        return res;
    }
};

} // namespace srpc





