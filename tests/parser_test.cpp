#include <srpc/element.hpp>
#include <srpc/parser.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>

namespace srpc {

// define statics
size_t trace::indent_level = 0;
bool trace::enable_trace = 0;
std::vector<std::shared_ptr<rpc_element>> contract::elements;
std::unordered_map<std::string, size_t> contract::element_index_map;

template <typename To, typename From>
std::unique_ptr<To> try_cast_unique(std::unique_ptr<From>& from, std::string err_msg) {
    if (auto raw = static_cast<To*>(from.get())) {
        from.release();
        return std::unique_ptr<To>(raw);
    }
    INFO("cast fail: "<<err_msg);
    return nullptr;
}

template <typename To, typename From>
std::shared_ptr<To> try_cast_shared(std::shared_ptr<From>& from, std::string err_msg) {
    auto casted = std::dynamic_pointer_cast<To>(from);
    if (!casted) {
        INFO("cast fail: " << err_msg);
    }
    return casted;
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
        contract::element_index_map.clear();
        std::string input = R"(
            message Request {
                string arg1;
                int32 arg2;
                bool arg3;
            }
        )";
        std::vector<field_descriptor> test_case {
            {1, "arg1", "std::string"}, 
            {1, "arg2", "int32_t"}, 
            {1, "arg3", "bool"}, 
        };

        lexer l(input);
        parser p(l); 
        p.parse_contract();
        check_parser_errors(p);

        REQUIRE(contract::elements.size() == 1);
        auto msg = try_cast_shared<message>(contract::elements[contract::element_index_map["Request"]], 
                "Error casting rpc element to message.");

        CHECK(msg->name == "Request");
        for (int i = 0; i < test_case.size(); i++) {
            auto field = msg->fields()[i].get();
            CHECK(field->name == test_case[i].name);
            CHECK(field->type == test_case[i].type);
        }
    }

    SECTION("Nested Message") {
        contract::elements.clear();
        contract::element_index_map.clear();
        std::string input = R"(
            message Engine {
                int8 pistons;
                int32 horsepower;
                char model;
            }

            message Car {
                Engine engine;
                string color;
            }
        )";
        std::vector<field_descriptor> engine_field_test_case {
            {1, "pistons", "int8_t"}, 
            {1, "horsepower", "int32_t"}, 
            {1, "model", "char"}, 
        };

        std::vector<field_descriptor> car_field_test_case {
            {0, "engine", "Engine"},
            {1, "color", "std::string"},
        };
        
        lexer l(input);
        parser p(l);
        p.parse_contract();
        check_parser_errors(p);

        REQUIRE(contract::elements.size() == 2);

        auto engine_msg = try_cast_shared<message>(contract::elements[contract::element_index_map["Engine"]], 
                "Error casting rpc element to message.");
        CHECK(engine_msg->name == "Engine");
        for (int i = 0; i < engine_field_test_case.size(); i++) {
            INFO("engine_field_test_case: "<<i);
            auto field = engine_msg->fields()[i].get();
            CHECK(field->name == engine_field_test_case[i].name);
            CHECK(field->type == engine_field_test_case[i].type);
        }

        auto car_msg = try_cast_shared<message>(contract::elements[contract::element_index_map["Car"]], 
                "Error casting rpc element to message.");
        CHECK(car_msg->name == "Car");
        for (int i = 0; i < car_field_test_case.size(); i++) {
            INFO("car_field_test_case: "<<i);
            auto field = car_msg->fields()[i].get();
            CHECK(field->name == car_field_test_case[i].name);
            CHECK(field->type == car_field_test_case[i].type);
        }
    }
}

TEST_CASE("Parse Service", "[parse][service]") {
    SECTION("Basic Service") {
        contract::elements.clear();
        contract::element_index_map.clear();
        std::string input = R"(
            service MyService {
                method SomeMethod(Request) returns (Response);
                method AnotherMethod(AnotherRequest) returns (AnotherResponse);
            }
        )";

        std::vector<method> my_service_test_case {
            {"SomeMethod", "Request", "Response"},
            {"AnotherMethod", "AnotherRequest", "AnotherResponse"},
        };

        lexer l(input);
        parser p(l);
        p.parse_contract();
        check_parser_errors(p);

        REQUIRE(contract::elements.size() == 1);

        auto svc = try_cast_shared<service>(contract::elements[contract::element_index_map["MyService"]], 
                "Error casting rpc element to message.");
        CHECK(svc->name == "MyService");

        for (int i = 0; i < my_service_test_case.size(); i++) {
            INFO("my_service_test_case: "<<i);
            auto method = svc->methods()[i].get();
            CHECK(method->name == my_service_test_case[i].name);
            CHECK(method->input_t == my_service_test_case[i].input_t);
            CHECK(method->output_t == my_service_test_case[i].output_t);
        }        
    }
}

} // namespace srpc
