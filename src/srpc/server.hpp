#pragma once 

#include "srpc/core.hpp"
#include "transport.hpp"
#include "packer.hpp"
#include <functional>
#include <unordered_map>

namespace srpc {

using function_base = std::function<std::unique_ptr<message_base>(const message_base&)>;

class server {
public:
    server() = default;
    ~server() = default;
 
    template <typename F>
    void register_method(std::string const& name, F func) {
        using input_type = typename function_traits<F>::input_type;
        using output_type = typename function_traits<F>::output_type;

        _function_registry[name] = [func] (const message_base& base_input) -> std::unique_ptr<message_base> {
            const auto& typed_input = static_cast<const input_type&>(base_input);
            output_type result = func(typed_input);
            return std::make_unique<output_type>(result);
        };
    }
    
    void register_service(servicer_base const& service) {
        std::apply(
            [this, &service] (const auto&... method) {
                (register_method(std::get<0>(method), std::get<1>(method)), ...);
            },
            servicer_base::methods
        );
    }
    
    void start(std::string const&& port) {
        int32_t listening_fd = transport::create_server_socket(port), accepted_fd;
        struct sockaddr_storage client_addr;
        socklen_t addr_size;

        while (true) {
            addr_size = sizeof(client_addr);
            if ((accepted_fd = accept(listening_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &addr_size)) < 0) {
                fprintf(stderr, "srpc::transport::create_server_socket(): accept failed.\n");
                continue;
            }

            size_t offset = 0;
            std::vector<uint8_t> packed = transport::recv_data(accepted_fd);

            // deserialize the method name and service name
            client_request req = packer::unpack_request(packed);
            auto it = _function_registry.find(req.method_name);
            if (it == _function_registry.end()) { return; } 
            
            // call the service's method
            auto called_function = it->second;
            std::vector<uint8_t> response = packer::pack_response(*(called_function(*(req.message_ptr))));

            transport::send_data(accepted_fd, response);
        }

        close(listening_fd);
    }

private:
    std::unordered_map<std::string, function_base> _function_registry; 
};

} //namespace srpc
