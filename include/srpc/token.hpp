#pragma once

#include <array>
#include <string>
#include <unordered_map>

namespace srpc {

enum class token_t : size_t {
    ILLEGAL     = 0,
    EOFT        ,

    IDENTIFIER  ,
    MESSAGE     ,
    SERVICE     ,
    METHOD      ,
    RETURNS     ,

    LBRACE      ,
    RBRACE      ,
    LPAREN      ,
    RPAREN      ,
    SEMICOLON   ,   

    INT8_T      ,
    INT16_T     ,
    INT32_T     ,
    INT64_T     ,
    CHAR_T      ,
    STRING_T    ,
    BOOL_T      ,

    INT_LIT     ,   

    COUNT
};

const std::unordered_map<std::string_view, token_t> keywords {
    {"message", token_t::MESSAGE},
    {"service", token_t::SERVICE},
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

const std::array<std::string, static_cast<size_t>(token_t::COUNT)> inv_map {
    "ILLEGAL", "EOFT",
    "IDENTIFIER", "MESSAGE", "SERVICE", "METHOD", "RETURNS"
    "LBRACE", "RBRACE", "LPAREN", "RPAREN", "SEMICOLON"
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
