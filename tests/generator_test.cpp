#include <srpc/lexer.hpp>
#include <srpc/parser.hpp>
#include <srpc/generator.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>

namespace srpc {

std::string remove_whitespace(const std::string& str) {
    std::string normalized;
    for (char c : str) {
        if (!std::isspace(c)) {
            normalized += c;
        }
    }
    return normalized;
}

TEST_CASE("generate header file message", "[generate][message]") {
    SECTION("primitive message") {
        contract::elements.clear();
        contract::element_index_map.clear();
        std::string input = R"(
            message Request {
                string arg1 = 1;
                int32 arg2 = 2;
            }
        )";
        lexer l(input);
        parser p(l); 
        p.parse_contract();
        REQUIRE(p.errors().size() == 0);

        auto msg = dynamic_pointer_cast<message>(contract::elements[contract::element_index_map["Request"]]);
        std::string res = remove_whitespace(generator::handle_message(msg));
        std::string expected = remove_whitespace(R"(
        struct Request : public srpc::message_base {
            std::string arg1;
            int32_t arg2;

            // overrides
            static constexpr const char* name = "Request";
            static constexpr auto fields = std::make_tuple(
                MESSAGE_FIELD(Request, arg1),
                MESSAGE_FIELD(Request, arg2)
            );
            void unpack(const std::vector<uint8_t>& packed, size_t& offset) override {
                int64_t header_length = 0;
                std::memcpy(&header_length, packed.data() + offset, sizeof(int64_t));
                offset += sizeof(int64_t);
                arg1 = std::string(reinterpret_cast<const char*>(packed.data() + offset), header_length);
                offset += sizeof(header_length);
                std::memcpy(&arg2, packed.data() + offset, sizeof(int32_t));
                offset += sizeof(int32_t);
            }
        };
        )");

        REQUIRE(res == expected);
    }

    SECTION("nested message") {
        contract::elements.clear();
        std::string input = R"(
            message nested_request {
                bool random_flag = 1;
                int64 i64 = 2;
            }
            message request {
                string arg1 = 1;
                int32 arg2 = 2;
                nested_request arg3 = 3;
            }
        )";
        lexer l(input);
        parser p(l); 
        p.parse_contract();
        REQUIRE(p.errors().size() == 0);

        auto msg = dynamic_pointer_cast<message>(contract::elements[contract::element_index_map["request"]]);
        std::string res = remove_whitespace(generator::handle_message(msg));

        std::string expected = remove_whitespace(R"(
        struct request : public srpc::message_base {
            std::string arg1;
            int32_t arg2;
            nested_request arg3;

            // overrides
            static constexpr const char* name = "request";
            static constexpr auto fields = std::make_tuple(
                MESSAGE_FIELD(request, arg1),
                MESSAGE_FIELD(request, arg2),
                MESSAGE_FIELD(request, arg3)
            );
            void unpack(const std::vector<uint8_t>& packed, size_t& offset) override {
                int64_t header_length = 0;
                std::memcpy(&header_length, packed.data() + offset, sizeof(int64_t));
                offset += sizeof(int64_t);
                arg1 = std::string(reinterpret_cast<const char*>(packed.data() + offset), header_length);
                offset += sizeof(header_length);
                std::memcpy(&arg2, packed.data() + offset, sizeof(int32_t));
                offset += sizeof(int32_t);
                nested_request arg3_;
                arg3_.unpack(packed, offset);
                arg3 = std::move(arg3_);
            }
        };)");

        REQUIRE(res == expected); 
    }
}

TEST_CASE("generate header file service", "[generate][service]") {
    SECTION("single method") {
        contract::elements.clear();
        contract::element_index_map.clear();
        std::string input = R"(
            service my_service {
                method some_method(request) returns (response);
            }
        )";
        lexer l(input);
        parser p(l); 
        p.parse_contract();
        REQUIRE(p.errors().size() == 0);

        std::string expected = remove_whitespace(R"(
        struct my_service_stub {
            response some_method(const request& req) {
                std::vector<uint8_t> packed = srpc::packer::pack(req);
            }
        };
        struct my_service_servicer {
            virtual response some_method(const request& req) = 0;
        };
        )");
        auto svc = dynamic_pointer_cast<service>(contract::elements[contract::element_index_map["my_service"]]);
        std::string res = remove_whitespace(generator::handle_service(svc));

        REQUIRE(expected == res); 
    }
}

TEST_CASE("write to file", "[generate][message][service][file]") {
    SECTION("generated message") {
        contract::elements.clear();
        contract::element_index_map.clear();
        std::string input = R"(
        message primitive_request {
            string str = 1;
            int64 i64 = 2;
        }
        message nested_request {
            bool random_flag = 1;
            int64 i64 = 2;
        }
        message request {
            string arg1 = 1;
            int32 arg2 = 2;
            nested_request arg3 = 3;
        }
        service my_service {
            method some_method(request) returns (primitive_request);
        }
        )";
        lexer l(input);
        parser p(l); 
        p.parse_contract();
        REQUIRE(p.errors().size() == 0);
        
        std::string path = "tests/stub/msg_srpc.hpp";
        generator::init_file(path);
        generator::handle_contract(path);
    }
}

}
