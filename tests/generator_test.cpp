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
                string arg1;
                int32 arg2;
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
                STRUCT_MEMBER(Request, arg1, "Request::arg1"),
                STRUCT_MEMBER(Request, arg2, "Request::arg2")
            );
            void unpack(srpc::buffer::ptr bp) override {
                srpc::packer p(bp);
                p >> arg1;
                p >> arg2;
            }
        };
        )");

        REQUIRE(res == expected);
    }

    SECTION("nested message") {
        contract::elements.clear();
        std::string input = R"(
            message nested_request {
                bool random_flag;
                int64 i64;
            }
            message request {
                string arg1;
                int32 arg2;
                nested_request arg3;
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
                STRUCT_MEMBER(request, arg1, "request::arg1"),
                STRUCT_MEMBER(request, arg2, "request::arg2"),
                STRUCT_MEMBER(request, arg3, "request::arg3")
            );
            void unpack(srpc::buffer::ptr bp) override {
                srpc::packer p(bp);
                p >> arg1;
                p >> arg2;
                arg3 = *(p->getv());
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
	        	if (!_init) {
	        		srpc::message_registry["request"] = []() -> std::unique_ptr<request> { 
                        return std::make_unique<request>(); 
                    };
	        		srpc::message_registry["response"] = []() -> std::unique_ptr<response> { 
                        return std::make_unique<response>(); 
                    };
	        	}
	        	_init = true;
	        }

	        void register_insecure_channel(std::string server_ip, std::string port) {
	        	if (_socket != -1) { close(_socket); }
	        	_socket = srpc::transport::create_client_socket(server_ip, port);
	        }

            response some_method(request& req) {
                srpc::packer pr;
                srpc::request_t<request> request;
                request.set_method_name("my_service_servicer::some_method");
                request.set_value(std::move(req));
                pr.pack_request(request);

		        srpc::transport::send_data(_socket, (*pr.buf()).data(), pr.size());
                srpc::message_t res = srpc::transport::recv_data(_socket);
                srpc::packer rpr(res.data(), res.size());
        
        		srpc::response_t<response> msg = rpr.unpack_response<response>(); 
        		return msg.value();
        	}

        private:
        	static bool _init;
	        int32_t     _socket= -1;
        };
        
        inline bool my_service_stub::_init = false;
        
        struct my_service_servicer : srpc::servicer_base {
        	virtual response some_method(request& req) { throw std::runtime_error("Method not implemented!"); }
            static constexpr const char* name = "my_service";
            static constexpr auto methods = std::make_tuple(
                STRUCT_MEMBER(my_service_servicer, some_method, "my_service_servicer::some_method")
            );
        };
        )");
        auto svc = dynamic_pointer_cast<service>(contract::elements[contract::element_index_map["my_service"]]);
        std::string res = remove_whitespace(generator::handle_service(svc));

        REQUIRE(res == expected); 
    }

    SECTION("generated message") {
        contract::elements.clear();
        contract::element_index_map.clear();
        std::string input = R"(
        message number {
            int64 num;
        }
        service calculate {
            method square(number) returns (number);
        }
        )";
        lexer l(input);
        parser p(l); 
        p.parse_contract();
        REQUIRE(p.errors().size() == 0);
        
        std::string path = "tests/stub/e2e_generated.hpp";
        generator::init_file(path);
        generator::handle_contract(path);
    } 
}

}
