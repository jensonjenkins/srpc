#pragma once

#include "core.hpp"
#include <cstdint>
#include <type_traits>
#include <vector>
#include <string>

namespace srpc {

struct serializer {

    template <typename T>
    static void serialize_arg(std::vector<uint8_t>& buffer, const T& arg) noexcept {
        if constexpr (std::is_base_of_v<message_base, T>) {
            std::vector<uint8_t> nested_buffer = serialize(arg);
            buffer.insert(buffer.end(), nested_buffer.begin(), nested_buffer.end()); // WARN: copies the nested_buffer
                                                                                     // TODO: probably avoid this later
        } else if constexpr(std::is_same_v<T, const char*>) {
            std::size_t length = std::strlen(arg);
            serialize_arg(buffer, length);
            for (const char* ptr = arg; *ptr != '\0'; ++ptr) {
                buffer.push_back(static_cast<uint8_t>(*ptr));
            }
        } else {
            const uint8_t* data = reinterpret_cast<const uint8_t*>(&arg);
            buffer.insert(buffer.end(), data, data + sizeof(T));
        }
    }
    
    template <>
    void serialize_arg<std::string>(std::vector<uint8_t>& buffer, const std::string& arg) noexcept {
        size_t length = arg.size();
        const uint8_t* start_addr = reinterpret_cast<const uint8_t*>(&length);
        buffer.insert(buffer.end(), start_addr, start_addr + sizeof(length));
        buffer.insert(buffer.end(), arg.begin(), arg.end());
    }

    template <typename T> 
    [[nodiscard]] static std::vector<uint8_t> serialize(const T& arg) {
        std::vector<uint8_t> buffer;
        
        serialize_arg(buffer, T::name);
        std::apply(
            [&buffer, &arg] (const auto&... field) {
                (serialize_arg(buffer, arg.*(field.second)), ...);
            },
            T::fields
        );

        return buffer;
    }

    template <typename... Args>
    [[nodiscard]] static std::vector<uint8_t> serialize(const std::string& name, Args&&... args) {
        std::vector<uint8_t> buffer;

        size_t name_length = name.size();
        const uint8_t* start_addr = reinterpret_cast<const uint8_t*>(&name_length);
        buffer.insert(buffer.end(), start_addr, start_addr + sizeof(name_length)); // serialize the size
        buffer.insert(buffer.end(), name.begin(), name.end()); // serialize the name

        (serialize_arg(buffer, std::forward<Args>(args)), ...);

        return buffer;
    }

};

} // namespace srpc
