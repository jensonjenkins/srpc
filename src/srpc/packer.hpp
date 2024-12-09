#pragma once

#include "core.hpp"
#include <cstdint>
#include <type_traits>
#include <vector>
#include <string>

namespace srpc {

struct packer {

    template <typename T>
    constexpr static void pack_arg(std::vector<uint8_t>& buffer, const T& arg) noexcept {
        if constexpr (std::is_base_of_v<message_base, T>) {
            std::vector<uint8_t> nested_buffer = pack(arg);
            buffer.insert(buffer.end(), nested_buffer.begin(), nested_buffer.end()); // WARN: copies the nested_buffer
                                                                                     // TODO: probably avoid this later
        } else {
            const uint8_t* data = reinterpret_cast<const uint8_t*>(&arg);
            buffer.insert(buffer.end(), data, data + sizeof(T));
        }
    }

    template <>
    void pack_arg<std::string>(std::vector<uint8_t>& buffer, const std::string& arg) noexcept {
        size_t length = arg.size();
        const uint8_t* start_addr = reinterpret_cast<const uint8_t*>(&length);
        buffer.insert(buffer.end(), start_addr, start_addr + sizeof(length));
        buffer.insert(buffer.end(), arg.begin(), arg.end());
    }

    // const char* const& might be a bit confusing. essentially its a const reference
    // to a char pointer in which you can't modify the object it is pointing to, nor can you modify
    // the pointer to point to something else.
    template <>
    void pack_arg<const char*>(std::vector<uint8_t>& buffer, const char* const& arg) noexcept {
        std::size_t length = std::strlen(arg);
        pack_arg(buffer, length);
        for (const char* ptr = arg; *ptr != '\0'; ++ptr) {
            buffer.push_back(static_cast<uint8_t>(*ptr));
        }
    }

    template <typename T> 
    [[nodiscard]] constexpr static std::vector<uint8_t> pack(const T& arg) {
        std::vector<uint8_t> buffer;
        
        pack_arg(buffer, T::name);
        std::apply(
            [&buffer, &arg] (const auto&... field) {
                (pack_arg(buffer, arg.*(field.second)), ...);
            },
            T::fields
        );

        return buffer;
    }
    
    template <typename T>
    [[nodiscard]] static T unpack(const std::vector<uint8_t>& packed) noexcept {
        size_t offset = 0;
        int64_t header_length = 0;
        std::memcpy(&header_length, packed.data() + offset, sizeof(int64_t));
        offset += sizeof(int64_t);
        
        std::string message_name(reinterpret_cast<const char*>(packed.data() + offset), header_length);
        offset += header_length;
        
        auto it = message_registry.find(message_name);
        if (it != message_registry.end()) {
            std::unique_ptr<message_base> instance = it->second();
        }
    }
};

} // namespace srpc





