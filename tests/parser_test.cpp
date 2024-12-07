#include "srpc/rpc_element.hpp"
#include <srpc/lexer.hpp>
#include <srpc/parser.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>

namespace srpc {

// define statics
size_t trace::indent_level = 0;
bool trace::enable_trace = 0;
std::unordered_map<std::string, std::unique_ptr<rpc_element>> contract::elements;

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
        contract::elements.clear();
        std::string input = R"(
            message Request {
                string arg1 = 1;
                optional int32 arg2 = 2;
                bool arg3 = 3;
            }
        )";
        std::vector<field_descriptor> test_case {
            {0, 1, "arg1", "str"}, 
            {1, 2, "arg2", "i32"}, 
            {0, 3, "arg3", "bool"}, 
        };

        lexer l(input);
        parser p(l); 
        p.parse_contract();
        check_parser_errors(p);

        REQUIRE(contract::elements.size() == 1);
        auto msg = try_cast_unique<message>(contract::elements["Request"], "Error casting rpc element to message.");

        CHECK(msg->name == "Request");
        for (int i = 0; i < test_case.size(); i++) {
            auto field = msg->fields()[i].get();
            CHECK(field->is_optional == test_case[i].is_optional);
            CHECK(field->field_number == test_case[i].field_number);
            CHECK(field->name == test_case[i].name);
            CHECK(field->type == test_case[i].type);
        }

    }

    SECTION("Nested Message") {
        contract::elements.clear();
        std::string input = R"(
            message Engine {
                int8 pistons = 1;
                int32 horsepower = 2;
                char model = 3;
            }

            message Car {
                Engine engine = 1;
                optional string color = 2;
            }
        )";
        std::vector<field_descriptor> engine_field_test_case {
            {0, 1, "pistons", "i8"}, 
            {0, 2, "horsepower", "i32"}, 
            {0, 3, "model", "char"}, 
        };

        std::vector<field_descriptor> car_field_test_case {
            {0, 1, "engine", "Engine"},
            {1, 2, "color", "str"},
        };
        
        lexer l(input);
        parser p(l);
        p.parse_contract();
        check_parser_errors(p);

        REQUIRE(contract::elements.size() == 2);

        auto engine_msg = try_cast_unique<message>(contract::elements["Engine"], "Error casting rpc element to message.");
        CHECK(engine_msg->name == "Engine");
        for (int i = 0; i < engine_field_test_case.size(); i++) {
            INFO("engine_field_test_case: "<<i);
            auto field = engine_msg->fields()[i].get();
            CHECK(field->is_optional == engine_field_test_case[i].is_optional);
            CHECK(field->field_number == engine_field_test_case[i].field_number);
            CHECK(field->name == engine_field_test_case[i].name);
            CHECK(field->type == engine_field_test_case[i].type);
        }

        auto car_msg = try_cast_unique<message>(contract::elements["Car"], "Error casting rpc element to message.");
        CHECK(car_msg->name == "Car");
        for (int i = 0; i < car_field_test_case.size(); i++) {
            INFO("car_field_test_case: "<<i);
            auto field = car_msg->fields()[i].get();
            CHECK(field->is_optional == car_field_test_case[i].is_optional);
            CHECK(field->field_number == car_field_test_case[i].field_number);
            CHECK(field->name == car_field_test_case[i].name);
            CHECK(field->type == car_field_test_case[i].type);
        }
    }
}

} // namespace srpc
