#pragma once

#include <tuple>
#include <functional>
#include <unordered_map>

namespace srpc {

#define MESSAGE_FIELD(type, field_name) std::make_tuple(#field_name, &type::field_name)

// to be inherited by generated messages
struct message_base {
    virtual ~message_base() = default;
    virtual void unpack(const std::vector<uint8_t>& packed, size_t offset) = 0;

    static constexpr const char* name = nullptr;
    static constexpr auto fields = std::make_tuple();
};

using message_factory = std::function<std::unique_ptr<message_base>()>;

// to be populated with user generated structs 
static std::unordered_map<std::string, message_factory> message_registry {};

} // namespace srpc
