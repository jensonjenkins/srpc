#include <srpc/transport.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>

#include <thread>

namespace srpc {

#define PORT "8080"

void run_server(std::string port) {
    int32_t accepted_fd = transport::create_server_socket(port);
    std::vector<uint8_t> response = transport::recv_data(accepted_fd);
    close(accepted_fd);

    CAPTURE(response);
    CHECK(false);
}

void run_client(std::string server_ip, std::string port, const std::vector<uint8_t>& data) {
    int32_t client_fd = transport::create_client_socket(server_ip, port);
    transport::send_data(client_fd, data);
    close(client_fd); 
}

TEST_CASE("send vector to server", "[socket][server][client]") {
    std::thread server_thread(run_server, PORT);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::vector<uint8_t> data = {65, 66, 67, 68, 69};
    run_client("127.0.0.1", PORT, data);
    server_thread.join();
}

} // namespace srpc
