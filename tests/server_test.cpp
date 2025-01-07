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
	void unpack(const std::vector<uint8_t>& packed, size_t& offset) override {
		int64_t header_length = 0;
		std::memcpy(&num, packed.data() + offset, sizeof(int64_t));
		offset += sizeof(int64_t);
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
    virtual number square(const number& req) {
        number out;
        out.num = req.num * req.num;
        return out;  
    }
};

namespace srpc {

TEST_CASE("register method", "[server][register][method]") {
    // server s;
    // calculator c;
    //
    // s.register_method("square", &calculate_servicer::square, c);
    //
    // number input, output;
    // input.num = 5;
    //
    // output = s.call<number>("square", input);
}

TEST_CASE("register service", "[server][register][service]") {

}

}
