#include "../src/srpc/lexer.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>

namespace srpc {

struct expected {
    token_t     expected_token_type;
    std::string expected_literal;
    expected(token_t e, std::string l) : expected_token_type(e), expected_literal(std::move(l)) {};
};

TEST_CASE("Symbol Test", "[symbol]") {
    std::string input = "{}=";
    std::vector<expected> test_case = {
        {token_t::LBRACE, "{"},
        {token_t::RBRACE, "}"},
        {token_t::ASSIGN, "="},
        {token_t::EOFT, ""}
    };

    lexer l(input);
    token cur_token("[UNSET]", token_t::ILLEGAL);
    for (int i = 0; i < test_case.size(); i++) {
        cur_token = l.next_token();
        INFO("test case number: "<<i);
        CAPTURE(l._input, l._input_len, l._cursor, l._peek_cursor, l._cur_char);
        REQUIRE(cur_token.literal == test_case[i].expected_literal);
        REQUIRE(cur_token.type == test_case[i].expected_token_type);
    }
}

TEST_CASE("Keyword Test", "[keyword]") {
    std::string input = "service message optional int8 int16 int32 int64 char string";
    std::vector<expected> test_case = {
        {token_t::SERVICE, "service"},
        {token_t::MESSAGE, "message"},
        {token_t::OPTIONAL, "optional"},
        {token_t::INT8_T, "int8"},
        {token_t::INT16_T, "int16"},
        {token_t::INT32_T, "int32"},
        {token_t::INT64_T, "int64"},
        {token_t::CHAR_T, "char"},
        {token_t::STRING_T, "string"},
        {token_t::EOFT, ""},
    };

    lexer l(input);
    token cur_token("[UNSET]", token_t::ILLEGAL);
    for (int i = 0; i < test_case.size(); i++) {
        cur_token = l.next_token();
        INFO("test case number: "<<i);
        CAPTURE(l._input, l._input_len, l._cursor, l._peek_cursor, l._cur_char);
        CAPTURE(cur_token.type, test_case[i].expected_token_type);
        REQUIRE(cur_token.literal == test_case[i].expected_literal);
        REQUIRE(cur_token.type == test_case[i].expected_token_type);
    }
}

} // namespace srpc

