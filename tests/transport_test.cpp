#include <srpc/transport.hpp>
#include <srpc/server.hpp>

#include <thread>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>


namespace srpc {

#define PORT "5555"

void run_server(std::string port) {
    int32_t listening_fd, accepted_fd;

    if ((listening_fd = transport::create_server_socket(port)) < 0) {
        fprintf(stderr, "run_server(): create server socket failed.\n");
        return;  
    }

    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof(client_addr);
    
    if ((accepted_fd = accept(listening_fd, 
                    reinterpret_cast<struct sockaddr*>(&client_addr), &addr_size)) < 0) {
        fprintf(stderr, "run_server(): accept failed.\n");
        return; 
    }

    message_t response = transport::recv_data(accepted_fd);
    const uint8_t expected[] = {70, 71, 72, 73, 74, 123};
    
    transport::send_data(accepted_fd, expected, 6);

    close(accepted_fd);
}

const uint8_t* run_client(std::string server_ip, std::string port, const uint8_t* data, size_t len) {
    int32_t client_fd; 
    if ((client_fd = transport::create_client_socket(server_ip, port)) < 0) {
        fprintf(stderr, "run_client(): create client socket failed.\n");
        return nullptr;   
    }
    transport::send_data(client_fd, data, len);
    
    message_t response = transport::recv_data(client_fd);
    close(client_fd); 

    return response.data();
}


TEST_CASE("send vector to server", "[socket][server][client]") {
    std::thread server_thread(run_server, PORT);
    std::this_thread::sleep_for(std::chrono::seconds(4));

    const uint8_t data[] = {65, 66, 67, 68, 69};
    const uint8_t expected[] = {70, 71, 72, 73, 74, 123};

    const uint8_t* res = run_client("127.0.0.1", PORT, data, 5);
    server_thread.join();

    REQUIRE(std::memcmp(expected, res, 6) == 0);
}

} // namespace srpc
