#include <srpc/core.hpp>
#include <srpc/packer.hpp>
#include <srpc/server.hpp>

#include <thread>
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

struct calculate_stub {
	calculate_stub() {
		if (!initialized) {
			srpc::message_registry["number"] = []() -> std::unique_ptr<number> {
				return std::make_unique<number>();
			};
		}
		initialized = true;
	}
    ~calculate_stub() { close(this->socket_fd); }

	void register_insecure_channel(std::string server_ip, std::string port) {
		if (socket_fd != -1) { close(socket_fd); }
		this->socket_fd = srpc::transport::create_client_socket(server_ip, port);
	}

	number square(number& req) {
        srpc::packer pr;
		srpc::request_t<number> request;
		request.set_method_name("calculate_servicer::square");
		request.set_value(std::move(req));
		pr.pack_request(request);

		srpc::transport::send_data(this->socket_fd, *pr.buf());
		std::vector<uint8_t> res = srpc::transport::recv_data(this->socket_fd);
        srpc::packer rpr(res);

		srpc::response_t<number> msg = rpr.unpack_response<number>();
		return msg.value();
	}
private:
	static bool initialized;
	int32_t socket_fd = -1;
};

inline bool calculate_stub::initialized = false;

void srpc::server::__testable_start(std::string const&& port) {
    int32_t listening_fd = transport::create_server_socket(port), accepted_fd;
    struct sockaddr_storage client_addr;
    socklen_t addr_size;

    addr_size = sizeof(client_addr);
    if ((accepted_fd = accept(listening_fd, 
                    reinterpret_cast<struct sockaddr*>(&client_addr), &addr_size)) < 0) {
        fprintf(stderr, "srpc::server::start(): accept failed.\n");
    }

    std::vector<uint8_t> bytes = transport::recv_data(accepted_fd);  

    // deserialize the method name and service name        
    packer::ptr p = std::make_shared<packer>(std::move(bytes));
    std::string funcname;
    (*p) >> funcname;

    // call the service's method
    packer::ptr r = call(funcname, p);
    assert(r->offset() == 0);

    std::vector<uint8_t> res(r->data(), r->data() + r->size());
    transport::send_data(accepted_fd, res);

    close(accepted_fd);
    close(listening_fd);
}

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
    
    s.register_service(c); 

    packer::ptr rp = s.call(funcname, p);
    response_t<number> response = rp->unpack_response<number>();
    
    REQUIRE(response.code() == RPC_SUCCESS);
    REQUIRE(response.value() == expected_value);
}

void run_server() {
    server s;
    calculator c;

    s.register_service(c); 
    s.__testable_start("8081"); 
}

TEST_CASE("start server", "[server]") {
    number input, expected, rcv;

    input.num = 5;
    expected.num = 25;

    std::thread server_thread(run_server);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    calculate_stub stub;
    stub.register_insecure_channel("127.0.0.1", "8081");
    rcv = stub.square(input); 

    server_thread.join();

    REQUIRE(rcv == expected);
}

} // namespace srpc


