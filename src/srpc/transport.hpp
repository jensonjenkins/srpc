#pragma once

#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <vector>

namespace srpc {

#ifndef SOCKET_SEND_FLAGS
#define SOCKET_SEND_FLAGS 0
#endif

#ifndef BACKLOG_SZ
#define BACKLOG_SZ 8
#endif

struct transport {

    [[nodiscard]] static int32_t create_server_socket(const std::string& port) { 
        int32_t status, listening_fd, accepted_fd;
        struct addrinfo hints, *servinfo;

        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;     // dont care ipv4 or ipv6
        hints.ai_socktype = SOCK_STREAM; // tcp, use DGRAM for udp
        hints.ai_flags = AI_PASSIVE;     // fill in my ip for me

        if ((status = getaddrinfo(nullptr, port.c_str(), &hints, &servinfo)) != 0) {
            fprintf(stderr, "srpc::transport::create_server_socket(): getaddrinfo error: %s\n" , gai_strerror(status));
            exit(EXIT_FAILURE);
        }

    if ((listening_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) < 0) {
        fprintf(stderr, "srpc::transport::create_server_socket(): error creating socket.\n");
        exit(EXIT_FAILURE);
    }
           
    if (bind(listening_fd, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
        fprintf(stderr, "srpc::transport::create_server_socket(): bind failed.\n");
        close(listening_fd);
        exit(EXIT_FAILURE);
    }

        if (listen(listening_fd, BACKLOG_SZ) < 0) { 
            fprintf(stderr, "srpc::transport::create_server_socket(): listen failed.\n");
            close(listening_fd);
            exit(EXIT_FAILURE);
        }

        return listening_fd;
    }

    [[nodiscard]] static int32_t create_client_socket(const std::string& server_ip, const std::string& port) {
        int32_t status, client_fd;
        struct addrinfo hints, *servinfo;

        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;     // dont care ipv4 or ipv6
        hints.ai_socktype = SOCK_STREAM; // tcp, use DGRAM for udp
    
        if ((status = getaddrinfo(nullptr, port.c_str(), &hints, &servinfo)) != 0) {
            fprintf(stderr, "srpc::transport::create_client_socket(): getaddrinfo error: %s\n" , gai_strerror(status));
            exit(EXIT_FAILURE);
        }

        if ((client_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) < 0) {
            fprintf(stderr, "srpc::transport::create_client_socket(): error creating socket.\n");
            exit(EXIT_FAILURE);
        }

        if (connect(client_fd, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
            fprintf(stderr, "srpc::transport::create_client_socket(): error connecting socket.\n");
            close(client_fd);
            exit(EXIT_FAILURE);
        }
        
        return client_fd;
    }

    static void send_data(int32_t socket_fd, const std::vector<uint8_t>& data) {
        uint32_t size = static_cast<uint32_t>(data.size());
        uint32_t size_network = htonl(size);
        if (send(socket_fd, &size_network, sizeof(size_network), SOCKET_SEND_FLAGS) != sizeof(size_network)) {
            fprintf(stderr, "srpc::transport::send_data(): failed to send data size.\n");
            return;
        }

        if (send(socket_fd, data.data(), size, SOCKET_SEND_FLAGS) != static_cast<ssize_t>(size)) {
            fprintf(stderr, "srpc::transport::send_data(): failed to send data payload.\n");
            return;
        }
    }

    [[nodiscard]] static std::vector<uint8_t> recv_data(int socket_fd) {
        uint32_t size_network;
        if (recv(socket_fd, &size_network, sizeof(size_network), MSG_WAITALL) != sizeof(size_network)) {
            fprintf(stderr, "srpc::transport::recv_data(): failed to receive data size.\n");
            return {};
        }

        uint32_t size = ntohl(size_network);

        std::vector<uint8_t> data(size);
        if (recv(socket_fd, data.data(), size, MSG_WAITALL) != static_cast<ssize_t>(size)) {
            fprintf(stderr, "srpc::transport::recv_data(): failed to receive data payload.\n");
            return {};
        }

        return data;
    }
};

} // namespace srpc
