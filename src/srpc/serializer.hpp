#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace srpc {

struct serializer {

    template <typename T>
    static void serialize_arg(std::vector<uint8_t>& buffer, const T& arg) noexcept {
        const uint8_t* data = reinterpret_cast<const uint8_t*>(&arg);
        buffer.insert(buffer.end(), data, data + sizeof(T));
    }
    
    template <>
    void serialize_arg<std::string>(std::vector<uint8_t>& buffer, const std::string& arg) noexcept {
        size_t length = arg.size();
        const uint8_t* start_addr = reinterpret_cast<const uint8_t*>(&length);
        buffer.insert(buffer.end(), start_addr, start_addr + sizeof(length));
        buffer.insert(buffer.end(), arg.begin(), arg.end());
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
