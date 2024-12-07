#include "srpc/rpc_element.hpp"
#include <srpc/lexer.hpp>
#include <srpc/parser.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>

namespace srpc {

size_t trace::indent_level = 0;
bool trace::enable_trace = 0;

template <typename To, typename From>
std::unique_ptr<To> try_cast_unique(std::unique_ptr<From>& from, std::string err_msg) {
    if (auto raw = static_cast<To*>(from.get())) {
        from.release();
        return std::unique_ptr<To>(raw);
    }
    INFO("cast fail: "<<err_msg);
    return nullptr;
}

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
        std::vector<field_descriptor> test_case {
            {0, 1, "arg1", typeid(std::string)}, 
            {1, 2, "arg2", typeid(int32_t)}, 
            {0, 3, "arg3", typeid(bool)}, 
        };

        lexer l(input);
        parser p(l); 
        contract* c = p.parse_contract();
        check_parser_errors(p);

        REQUIRE(c->elements.size() == 1);
        auto msg = try_cast_unique<message>(c->elements[0], "");

        REQUIRE(msg->name == "Request");
        for (int i = 0; i < test_case.size(); i++) {
            auto field = msg->fields()[i].get();
            REQUIRE(field->is_optional == test_case[i].is_optional);
            REQUIRE(field->field_number == test_case[i].field_number);
            REQUIRE(field->name == test_case[i].name);
            REQUIRE(field->type == test_case[i].type);
        }

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
