#pragma once

#include "token.hpp"
#include <cctype>
#include <cstdint>
#include <string>

namespace srpc {

/**
 * Parse raw contract files into tokens
 */
class lexer {
private:
    std::string     _input;
    std::uint32_t   _input_len;
    std::uint32_t   _cursor;
    std::uint32_t   _peek_cursor;
    char            _cur_char;

public:
    lexer() = delete;
    lexer(std::string input) : _input(input), _input_len(_input.size()), _peek_cursor(0) {}
    ~lexer() {}
    
    constexpr void read_char() {
        if(_peek_cursor >= _input_len) {
            _cur_char = 0;
        } else {
            _cur_char = _input[_peek_cursor];
        }
        _cursor = _peek_cursor;
        ++_peek_cursor;
    }

    token next_token() {
        read_char();
        if (std::isspace(_cur_char)) {
            handle_whitespace();
        }
        token cur_token("[UNSET]", token_t::ILLEGAL);
        
        switch(_cur_char) {
        case '=':
            cur_token.literal = "=";
            cur_token.type = token_t::ASSIGN;
            break;
        case '{':
            cur_token.literal = "{";
            cur_token.type = token_t::LBRACE;
            break;
        case '}':
            cur_token.literal = "}";
            cur_token.type = token_t::RBRACE;
            break;
        case '(':
            cur_token.literal = "(";
            cur_token.type = token_t::LPAREN;
            break;
        case ')':
            cur_token.literal = ")";
            cur_token.type = token_t::RPAREN;
            break;
        case ';':
            cur_token.literal = ";";
            cur_token.type = token_t::SEMICOLON;
            break;
        case 0:
            cur_token.literal = "";
            cur_token.type = token_t::EOFT;
            break;
        default:
            if(is_letter(_cur_char)) {
                std::string ident = read_identifier();
                cur_token.literal = ident;
                cur_token.type = lookup_ident(ident);
            } else if (std::isdigit(_cur_char)) {
                std::string digits = read_digits();
                cur_token.literal = digits;
                cur_token.type = token_t::INT_LIT;
            } else {
                cur_token.literal = "[UNRECOGNIZED]";
                cur_token.type = token_t::ILLEGAL;
            }
        }
        return cur_token;
    }

private:
    char peek_char() const noexcept {
        if(_peek_cursor >= _input_len) {   
            return 0;
        } else {
            return _input[_peek_cursor];
        }
    }

    std::string read_identifier() noexcept {
        std::uint32_t start = _cursor;
        while(is_letter(peek_char()) || std::isdigit(peek_char())) { 
            read_char(); 
        }
        std::string identifier = _input.substr(start, _cursor - start + 1);

        return identifier;
    }

    std::string read_digits() noexcept {
        std::uint32_t start = _cursor;
        while(std::isdigit(peek_char())) { 
            read_char(); 
        }
        std::string digits = _input.substr(start, _cursor - start + 1);

        return digits;
    }

    token_t lookup_ident(std::string& ident) noexcept {
        auto it = keywords.find(ident);
        if (it != keywords.end()) {
            return it->second;
        }
        return token_t::IDENTIFIER;
    }


    constexpr bool is_letter(char ch) const noexcept {
        return 'a' <= ch && ch <= 'z' || 'A' <= ch && ch <= 'Z' || ch == '_';
    }

    constexpr void handle_whitespace() noexcept {
        while(std::isspace(_cur_char)) {
            read_char();
        }
    }
};

} // namespace srpc

 
