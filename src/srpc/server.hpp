#pragma once 

#include "srpc/core.hpp"
#include "transport.hpp"
#include "packer.hpp"
#include <functional>
#include <unordered_map>

namespace srpc {

class server {
public:
    server() = default;
    ~server() = default;
    
    template <typename F, typename C>
    void register_method(std::string const& name, F func, C& instance) {
        using input_type = typename function_traits<F>::input_type;
        using return_type = typename function_traits<F>::return_type;
        static_assert(std::is_base_of_v<message_base, std::decay_t<input_type>>);
        static_assert(std::is_base_of_v<message_base, std::decay_t<return_type>>);

        _function_registry[name] = std::bind(&call_proxy<F>, this, func, std::placeholders::_1);
    }
    
    template <typename R, typename Arg>
    R call(std::string name, Arg const& arg) {
    }

    template <typename F>
    void call_proxy(F func, std::vector<uint8_t> const& data) { call_proxy_impl(func, data); }
    
    template <typename R, typename C, typename I>
    void call_proxy_impl(R (C::*func)(const I&), I const& arg) {
        call_proxy_impl(std::function<R(const I&)>(func), arg);
    }

    template <typename R, typename I>
    void call_proxy_impl(std::function<R(const I&)> func, I const& arg) {
		R result = call_helper<R>(func, arg);

		response_t<R> response;
		response.set_code(RPC_SUCCESS);
		response.set_value(result);
		// (*pr) << response;
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
            // call the service's method
        }

        close(listening_fd);
    }

private:
    std::unordered_map<std::string, std::function<void(std::vector<uint8_t>)>> _function_registry; 
};

} //namespace srpc
