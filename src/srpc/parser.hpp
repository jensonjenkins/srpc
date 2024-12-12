#pragma once

#include "element.hpp"
#include "lexer.hpp"
#include "token.hpp"
#include "trace.hpp"
#include <string>

namespace srpc {

#ifndef FUNCTION_TRACE
#define FUNCTION_TRACE trace t(std::string(__FUNCTION__));
#endif

/**
 * Parse tokens into rpc_elements, and grouping them into contracts
 */
class parser {
private:
    lexer                       _l;
    token                       _cur_token;
    token                       _peek_token;
    std::vector<std::string>    _errors;
 
public:
    void next_token() noexcept {
        _cur_token = _peek_token;
        _peek_token = _l.next_token();
    }

    parser(lexer l) noexcept : _l(l), _cur_token(), _peek_token() {
        next_token();
        next_token(); // initialize both _cur and _peek tokens
    }

    void parse_contract() noexcept { 
        FUNCTION_TRACE;

        while(_cur_token.type != token_t::EOFT){
            rpc_element* e = parse_element();
            if (e) { contract::add_element(std::move(e)); }
            next_token();
        }
    }

    rpc_element* parse_element() {
        FUNCTION_TRACE;

        switch (_cur_token.type) {
        case token_t::MESSAGE:
            return parse_message();
            break;
        case token_t::SERVICE:
            return parse_service();
            break;
        default:
            _errors.push_back("Unrecognized rpc_element token: " + inv_map[static_cast<size_t>(_cur_token.type)]);
            return nullptr;
        }
    }

    message* parse_message() noexcept { 
        FUNCTION_TRACE;

        message* msg = new message(_cur_token);
        if (!expect_peek(token_t::IDENTIFIER)) { return nullptr; }

        msg->name = _cur_token.literal;

        if (!expect_peek(token_t::LBRACE)) { return nullptr; }
        next_token();

        while(_cur_token.type != token_t::RBRACE) {
            field_descriptor* fd = parse_message_field();
            if (fd != nullptr) { msg->add_field_descriptor(std::move(fd)); }
        }

        return msg; 
    }

    [[nodiscard]] service* parse_service() noexcept { 
        FUNCTION_TRACE;

        service* svc = new service(_cur_token);
        if (!expect_peek(token_t::IDENTIFIER)) { return nullptr; }

        svc->name = _cur_token.literal;

        if (!expect_peek(token_t::LBRACE)) { return nullptr; }
        next_token();

        while(_cur_token.type != token_t::RBRACE) {
            method* mtd = parse_method();
            if (mtd != nullptr) { svc->add_method(mtd); }
        }
        return svc; 
    }

    [[nodiscard]] method* parse_method() noexcept {
        FUNCTION_TRACE;

        method* mtd = new method;
        if (!cur_token_is(token_t::METHOD)) { return nullptr; }
        if (!expect_peek(token_t::IDENTIFIER)) { return nullptr; }

        mtd->name = _cur_token.literal;

        if (!expect_peek(token_t::LPAREN)) { return nullptr; }
        if (!expect_peek(token_t::IDENTIFIER)) { return nullptr; }

        mtd->input_t = _cur_token.literal;

        if (!expect_peek(token_t::RPAREN)) { return nullptr; }
        if (!expect_peek(token_t::RETURNS)) { return nullptr; }
        if (!expect_peek(token_t::LPAREN)) { return nullptr; }
        if (!expect_peek(token_t::IDENTIFIER)) { return nullptr; }

        mtd->output_t= _cur_token.literal;

        if (!expect_peek(token_t::RPAREN)) { return nullptr; }
        if (!expect_peek(token_t::SEMICOLON)) { return nullptr; }
        next_token();
        
        return mtd;
    }

    [[nodiscard]] field_descriptor* parse_message_field() noexcept {
        FUNCTION_TRACE;

        field_descriptor* fd = new field_descriptor;
        if (cur_token_is(token_t::OPTIONAL)) {
            fd->is_optional = true;
            next_token();
        }

        fd->is_primitive = 1;
        switch (_cur_token.type) {
        case token_t::BOOL_T:
            fd->type = "bool";
            break;
        case token_t::INT8_T:
            fd->type = "int8_t";
            break;
        case token_t::INT16_T:
            fd->type = "int16_t";
            break;
        case token_t::INT32_T:
            fd->type = "int32_t";
            break;
        case token_t::INT64_T:
            fd->type = "int64_t";
            break;
        case token_t::STRING_T:
            fd->type = "std::string";
            break;
        case token_t::CHAR_T:
            fd->type = "char";
            break;
        case token_t::IDENTIFIER:
            {
                fd->is_primitive = 0;
                auto it = contract::element_index_map.find(_cur_token.literal);
                if (it != contract::element_index_map.end()) {
                    fd->type = _cur_token.literal;
                } else {
                    _errors.push_back("Undefined identifier in field type.");
                    return nullptr;
                }
                break;
            }
        default:
            _errors.push_back("Expected cur_token to be a field_type.");
            return nullptr;
        }

        if (!expect_peek(token_t::IDENTIFIER)) { return nullptr; }

        fd->name = _cur_token.literal;

        if (!expect_peek(token_t::ASSIGN)) { return nullptr; }
        if (!expect_peek(token_t::INT_LIT)) { return nullptr; }
        
        fd->field_number = std::stoi(_cur_token.literal);

        if (!expect_peek(token_t::SEMICOLON)) { return nullptr; }
        next_token();
    
        return fd;
    }
    
    bool expect_peek(token_t token_type) noexcept {
        if (peek_token_is(token_type)) {
            next_token();
            return true;
        } else {
            peek_error(token_type);
            return false;
        }
    }

    void peek_error(token_t token_type) noexcept {
        std::string error_msg;
        error_msg = "expected next token to be " 
            + inv_map[static_cast<size_t>(token_type)]
            + ", got " 
            + inv_map[static_cast<size_t>(_peek_token.type)]
            + " instead.";
        _errors.push_back(error_msg);
    }

    std::vector<std::string>& errors() noexcept { return _errors; }

    bool cur_token_is(token_t token_type) const noexcept { return _cur_token.type == token_type; }

    bool peek_token_is(token_t token_type) const noexcept { return _peek_token.type == token_type; }

};

} // namespace srpc 

