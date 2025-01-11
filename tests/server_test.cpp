#include <srpc/core.hpp>
#include <srpc/packer.hpp>
#include <srpc/server.hpp>
#include <iostream>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>

struct number : public srpc::message_base {
	int64_t num;
    
	// overrides
	static constexpr const char* name = "number";
	static constexpr auto fields = std::make_tuple(
		STRUCT_MEMBER(number, num, "number::num")
	);
	void unpack(srpc::buffer::ptr bp) override {
        srpc::packer p(bp);
        p >> num;
	}

    constexpr bool operator==(const number& other) const noexcept { return other.num == num; }
};

struct calculate_servicer : srpc::servicer_base {
	virtual number square(number& req) { throw std::runtime_error("Method not implemented!"); }

	static constexpr const char* name = "calculate";
	static constexpr auto methods = std::make_tuple(
		STRUCT_MEMBER(calculate_servicer, square, "calculate_servicer::square")
	);
};

struct calculator : public calculate_servicer {
    virtual number square(number& req) override {
        number out;
        out.num = req.num * req.num;
        return out;  
    }
};

namespace srpc {

TEST_CASE("call registered method", "[call][register][method]") {
    srpc::message_registry["number"] = []() -> std::unique_ptr<number> { return std::make_unique<number>(); };

    server s;
    number input, expected_value;
    request_t<number> req;
    packer::ptr p = std::make_shared<packer>();
    calculator c;
    
    input.num = 5;
    expected_value.num = 25;

    req.set_value(std::move(input));
    req.set_method_name("calculator::square");
    p->pack_request(req);
    
    std::string funcname;
    (*p) >> funcname;

    s.register_method(funcname, &calculator::square, c);

    packer::ptr rp = s.call(funcname, p);
    response_t<number> response = rp->unpack_response<number>();
    
    REQUIRE(response.code() == RPC_SUCCESS);
    REQUIRE(response.value() == expected_value);
}

TEST_CASE("register service", "[server][register][service]") {
    srpc::message_registry["number"] = []() -> std::unique_ptr<number> { return std::make_unique<number>(); };

    server s;
    number input, expected_value;
    request_t<number> req;
    packer::ptr p = std::make_shared<packer>();
    calculator c;

    input.num = 5;
    expected_value.num = 25;

    req.set_value(std::move(input));
    req.set_method_name("calculate_servicer::square");
    p->pack_request(req);

    std::string funcname;
    (*p) >> funcname;
    
    std::cout<<std::get<0>(std::get<0>(calculator::methods))<<std::endl;
    s.register_service(c); 

    packer::ptr rp = s.call(funcname, p);
    response_t<number> response = rp->unpack_response<number>();
    
    REQUIRE(response.code() == RPC_SUCCESS);
    REQUIRE(response.value() == expected_value);
}

}


