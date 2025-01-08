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
	        my_service_stub() {
	        	if (!initialized) {
	        		srpc::message_registry["request"] = []() -> std::unique_ptr<request> { 
                        return std::make_unique<request>(); 
                    };
	        		srpc::message_registry["response"] = []() -> std::unique_ptr<response> { 
                        return std::make_unique<response>(); 
                    };
	        	}
	        	initialized = true;
	        }

	        void register_insecure_channel(std::string server_ip, std::string port) {
	        	if (socket_fd != -1) { close(socket_fd); }
	        	this->socket_fd = srpc::transport::create_client_socket(server_ip, port);
	        }

            response some_method(request& req) {
                srpc::request_t<number> request;
                request.set_method_name("my_service::some_method");
                request.set_value(std::move(req));

        		srpc::buffer packed = srpc::packer::pack_request(request);
        		srpc::transport::send_data(this->socket_fd, packed);
        		std::vector<uint8_t> res = srpc::transport::recv_data(this->socket_fd);
        
        		srpc::response_t<response> msg = srpc::packer::unpack_response<response>(res); 
        		return msg.value();
        	}

        private:
        	static bool initialized;
	        int32_t socket_fd = -1;
        };
        
        inline bool my_service_stub::initialized = false;
        
        struct my_service_servicer : srpc::servicer_base {
        	virtual response some_method(const request& req) { throw std::runtime_error("Method not implemented!"); }
            static constexpr const char* name = "my_service";
            static constexpr auto fields = std::make_tuple(
                SERVICE_METHOD(my_service_servicer, some_method)
            );
        };
        )");
        auto svc = dynamic_pointer_cast<service>(contract::elements[contract::element_index_map["my_service"]]);
        std::string res = remove_whitespace(generator::handle_service(svc));

        REQUIRE(expected == res); 
    }

    SECTION("generated message") {
        contract::elements.clear();
        contract::element_index_map.clear();
        std::string input = R"(
        message number {
            int64 num = 1;
        }
        service calculate {
            method square(number) returns (number);
        }
        )";
        lexer l(input);
        parser p(l); 
        p.parse_contract();
        REQUIRE(p.errors().size() == 0);
        
        // std::string path = "tests/stub/e2e_generated.hpp";
        // generator::init_file(path);
        // generator::handle_contract(path);
    } 
}

}
