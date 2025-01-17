#include <srpc/parser.hpp>

#include <catch2/catch_test_macros.hpp>
#include <string>

namespace srpc {

struct expected {
    token_t     expected_token_type;
    std::string expected_literal;
    expected(token_t e, std::string l) : expected_token_type(e), expected_literal(std::move(l)) {};
};

TEST_CASE("Symbol Test", "[symbol]") {
    std::string input = "{}";
    std::vector<expected> test_case = {
        {token_t::LBRACE, "{"},
        {token_t::RBRACE, "}"},
        {token_t::EOFT, ""}
    };

    lexer l(input);
    token cur_token;
    for (int i = 0; i < test_case.size(); i++) {
        cur_token = l.next_token();
        INFO("token number: "<<i);
        // CAPTURE(l._input, l._input_len, l._cursor, l._peek_cursor, l._cur_char);
        REQUIRE(cur_token.literal == test_case[i].expected_literal);
        REQUIRE(cur_token.type == test_case[i].expected_token_type);
    }
}

TEST_CASE("Keyword Test", "[keyword]") {
    std::string input = "service message int8 int16 int32 int64 char string";
    std::vector<expected> test_case = {
        {token_t::SERVICE, "service"},
        {token_t::MESSAGE, "message"},
        {token_t::INT8_T, "int8"},
        {token_t::INT16_T, "int16"},
        {token_t::INT32_T, "int32"},
        {token_t::INT64_T, "int64"},
        {token_t::CHAR_T, "char"},
        {token_t::STRING_T, "string"},
        {token_t::EOFT, ""},
    };

    lexer l(input);
    token cur_token;
    for (int i = 0; i < test_case.size(); i++) {
        cur_token = l.next_token();
        INFO("token number: "<<i);
        // CAPTURE(l._input, l._input_len, l._cursor, l._peek_cursor, l._cur_char);
        REQUIRE(cur_token.literal == test_case[i].expected_literal);
        REQUIRE(cur_token.type == test_case[i].expected_token_type);
    }
}

TEST_CASE("Message", "[message]") {
    SECTION("Basic Message") {
        std::string input = R"(
            message Request {
                string arg1;
                int32 arg2;
                bool arg3;
            }
        )";
        std::vector<expected> test_case = {
            {token_t::MESSAGE, "message"},
            {token_t::IDENTIFIER, "Request"},
            {token_t::LBRACE, "{"},

            {token_t::STRING_T, "string"},
            {token_t::IDENTIFIER, "arg1"},
            {token_t::SEMICOLON, ";"},

            {token_t::INT32_T, "int32"},
            {token_t::IDENTIFIER, "arg2"},
            {token_t::SEMICOLON, ";"},

            {token_t::BOOL_T, "bool"},
            {token_t::IDENTIFIER, "arg3"},
            {token_t::SEMICOLON, ";"},

            {token_t::RBRACE, "}"},
            {token_t::EOFT, ""},
        };

        lexer l(input);
        token cur_token;
        for (int i = 0; i < test_case.size(); i++) {
            cur_token = l.next_token();
            INFO("token number: "<<i);
            // CAPTURE(l._input, l._input_len, l._cursor, l._peek_cursor, l._cur_char);
            REQUIRE(cur_token.literal == test_case[i].expected_literal);
            REQUIRE(cur_token.type == test_case[i].expected_token_type);
        }    
    }
}

TEST_CASE("Service", "[service]") {
    SECTION("Basic Service") {
        std::string input = R"(
            service MyService {
                method SomeMethod(Request) returns (Response);
            }
        )";
        std::vector<expected> test_case = {
            {token_t::SERVICE, "service"},
            {token_t::IDENTIFIER, "MyService"},
            {token_t::LBRACE, "{"},

            {token_t::METHOD, "method"},
            {token_t::IDENTIFIER, "SomeMethod"},
            {token_t::LPAREN, "("},
            {token_t::IDENTIFIER, "Request"},
            {token_t::RPAREN, ")"},
            {token_t::RETURNS, "returns"},
            {token_t::LPAREN, "("},
            {token_t::IDENTIFIER, "Response"},
            {token_t::RPAREN, ")"},
            {token_t::SEMICOLON, ";"},

            {token_t::RBRACE, "}"},
            {token_t::EOFT, ""},
        };

        lexer l(input);
        token cur_token;
        for (int i = 0; i < test_case.size(); i++) {
            cur_token = l.next_token();
            INFO("token number: "<<i);
            // CAPTURE(l._input, l._input_len, l._cursor, l._peek_cursor, l._cur_char);
            REQUIRE(cur_token.literal == test_case[i].expected_literal);
            REQUIRE(cur_token.type == test_case[i].expected_token_type);
        }    
    }
    
}

} // namespace srpc

