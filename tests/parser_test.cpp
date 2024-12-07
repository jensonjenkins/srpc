#include <srpc/lexer.hpp>
#include <srpc/parser.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>

namespace srpc {

size_t trace::indent_level = 0;
bool trace::enable_trace = 0;

void check_parser_errors(parser& p) {
    std::vector<std::string> errors = p.errors();
    for (std::string_view error : errors) {
        INFO("parser error: " << error);
    }
    REQUIRE(errors.size() == 0);
}

TEST_CASE("Parse Message", "[parse][message]") {
    SECTION("Basic Message") {
        std::string input = R"(
            message Request {
                string arg1 = 1;
                optional int32 arg2 = 2;
                bool arg3 = 3;
            }
        )";
        lexer l(input);
        parser p(l); 
        contract* c = p.parse_contract();
        check_parser_errors(p);

        REQUIRE(c->element_map.size() == 1);
    }

    SECTION("Nested Message") {
        std::string input = R"(
            message Engine {
                int8 pistons = 1;
                int32 horsepower = 2;
                string model = 3;
            }

            message Car {
                Engine engine = 1;
            }
        )";
        lexer l(input);
        parser p(l);  
    }
}

} // namespace srpc
