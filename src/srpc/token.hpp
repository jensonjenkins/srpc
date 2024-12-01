#pragma once

#include <array>
#include <string>
#include <unordered_map>

namespace srpc {

enum class token_t : std::size_t {
    ILLEGAL     = 0,
    EOFT        = 1,

    IDENTIFIER  = 2,
    MESSAGE     = 3,
    SERVICE     = 4,
    OPTIONAL    = 5,
    METHOD      = 6,
    RETURNS     = 7,

    LBRACE      = 8,
    RBRACE      = 9,
    LPAREN      = 10,
    RPAREN      = 11,
    ASSIGN      = 12,
    SEMICOLON   = 13,   

    INT8_T      = 14,
    INT16_T     = 15,
    INT32_T     = 16,
    INT64_T     = 17,
    CHAR_T      = 18,
    STRING_T    = 19,
    BOOL_T      = 20,

    INT_LIT     = 21,   

    COUNT
};

const std::unordered_map<std::string_view, token_t> keywords {
    {"message", token_t::MESSAGE},
    {"service", token_t::SERVICE},
    {"optional", token_t::OPTIONAL},
    {"method", token_t::METHOD},
    {"returns", token_t::RETURNS},
    {"int8", token_t::INT8_T},
    {"int16", token_t::INT16_T},
    {"int32", token_t::INT32_T},
    {"int64", token_t::INT64_T},
    {"char", token_t::CHAR_T},
    {"string", token_t::STRING_T},
    {"bool", token_t::BOOL_T},
}; 

const std::array<std::string, static_cast<std::size_t>(token_t::COUNT)> inv_map {
    "ILLEGAL", "EOFT",
    "IDENTIFIER", "MESSAGE", "SERVICE", "OPTIONAL", "METHOD", "RETURNS"
    "LBRACE", "RBRACE", "LPAREN", "RPAREN", "ASSIGN", "SEMICOLON"
    "INT8_T", "INT16_T", "INT32_T", "INT64_T", "CHAR_T", "STRING_T", "BOOL_T",
    "INT_LIT"
};

struct token {
    std::string literal;
    token_t     type;
    
    token() : literal("[UNSET]"), type(token_t::ILLEGAL) {}
    explicit token(std::string lit, token_t t) noexcept : literal(lit), type(t) {}
};

} // namespace srpc
