#include <srpc/server.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>

struct number : public srpc::message_base {
	int64_t num;

	// overrides
	static constexpr const char* name = "number";
	static constexpr auto fields = std::make_tuple(
		MESSAGE_FIELD(number, num)
	);
	void unpack(srpc::buffer::ptr bp) override {
        srpc::packer p(bp);
        p >> num;
	}
};

struct calculate_servicer : srpc::servicer_base {
	virtual number square(const number& req) { throw std::runtime_error("Method not implemented!"); }

	static constexpr const char* name = "calculate";
	static constexpr auto fields = std::make_tuple(
		SERVICE_METHOD(calculate_servicer, square)
	);
};

struct calculator : public calculate_servicer {
    virtual number square(number& req) {
        number out;
        out.num = req.num * req.num;
        return out;  
    }
};

namespace srpc {

TEST_CASE("register method", "[server][register][method]") {
    server s;
    calculator calc_instance;
    s.register_method("calculator::square", &calculator::square, calc_instance);
}

TEST_CASE("register service", "[server][register][service]") {

}

}
