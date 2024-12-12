#include <srpc/transport.hpp>
#include <srpc/server.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>


namespace srpc {

#define PORT "8080"

// std::vector<uint8_t> run_server(std::string port) {
//     server s;
//     s.start(PORT, 1);
// }
//
// void run_client(std::string server_ip, std::string port, const std::vector<uint8_t>& data) {
//     int32_t client_fd = transport::create_client_socket(server_ip, port);
//     transport::send_data(client_fd, data);
//     close(client_fd); 
// }

TEST_CASE("send vector to server", "[socket][server][client]") {
}

} // namespace srpc
