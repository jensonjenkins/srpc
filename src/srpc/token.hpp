#pragma once

#include <array>
#include <string>
#include <cstdint>
#include <unordered_map>

namespace srpc {

constexpr std::uint8_t token_count = 16;
enum class token_t : std::uint8_t {
    ILLEGAL     = 0,
    EOFT        = 1,

    MESSAGE     = 2,
    SERVICE     = 3,
    OPTIONAL    = 4,
    IDENTIFIER  = 5,
    LBRACE      = 6,
    RBRACE      = 7,
    ASSIGN      = 8,

    INT8_T      = 9,
    INT16_T     = 10,
    INT32_T     = 11,
    INT64_T     = 12,
    CHAR_T      = 13,
    STRING_T    = 14,

    INT_LIT     = 15,   
};

const std::unordered_map<std::string_view, token_t> keywords {
    {"message", token_t::MESSAGE},
    {"service", token_t::SERVICE},
    {"optional", token_t::OPTIONAL},
    {"int8", token_t::INT8_T},
    {"int16", token_t::INT16_T},
    {"int32", token_t::INT32_T},
    {"int64", token_t::INT64_T},
    {"char", token_t::CHAR_T},
    {"string", token_t::STRING_T},
}; 

const std::array<std::string, token_count> inv_map {
    "ILLEGAL", "EOFT", 
    "MESSAGE", "SERVICE", "OPTIONAL", "IDENTIFIER", "LBRACE", "RBRACE", "ASSIGN",
    "INT8_T", "INT16_T", "INT32_T", "INT64_T", "CHAR_T", "STRING_T", "INT_LITERAL"
};

struct token {
    std::string literal;
    token_t     type;

    explicit token(std::string lit, token_t t) noexcept : literal(lit), type(t) {}
};

} // namespace srpc
