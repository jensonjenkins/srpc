#pragma once

#include <tuple>

namespace srpc {

#define MESSAGE_FIELD(type, field_name) std::make_pair(#field_name, &type::field_name)

// to be inherited by generated messages
struct message_base {
    static constexpr const char* name = nullptr;
    static constexpr auto fields = std::make_tuple();
};

} // namespace srpc
