#pragma once 

#include "core.hpp"
#include "transport.hpp"
#include "packer.hpp"
#include <functional>
#include <unordered_map>

namespace srpc {

#ifndef METHOD_NAME_IDX
#define METHOD_NAME_IDX 0
#endif

#ifndef FUNC_PTR_IDX 
#define FUNC_PTR_IDX 1
#endif

class server {
public:
    server() = default;
    ~server() = default;
    
    template <typename F, typename S>
    void register_method(std::string const& name, F func, S& instance) {
        using input_type = typename function_traits<F>::input_type;
        using return_type = typename function_traits<F>::return_type;
        static_assert(std::is_base_of_v<message_base, std::decay_t<input_type>>);
        static_assert(std::is_base_of_v<message_base, std::decay_t<return_type>>);

        _function_registry[name] = std::bind(
                &server::call_proxy<F, S>, this, func, instance, std::placeholders::_1, std::placeholders::_2);
    }
 
    packer::ptr call(std::string const& funcname, packer::ptr p) {
        packer::ptr rp = std::make_shared<packer>(); // packer to populate with return value

        auto it = _function_registry.find(funcname);
        if (it == _function_registry.end()) {
            fprintf(stderr, "srpc::server::call(): function not registerred.\n");
            (*rp) << static_cast<uint8_t>(RPC_ERR_FUNCTION_NOT_REGISTERRED);
        }

        auto func = it->second;
        func(rp.get(), p.get());
            
        return rp;
    }
    
    /// @tparam S servicer class
    /// @tparam F member function of S
    /// @param func member function pointer of type F
    /// @param instance instance of S
    /// @param rp packer to be populated with return value (return packer)
    /// @param cp packer containing data to be read from (client packer)
    template <typename F, typename S>
    void call_proxy(F func, S& instance, packer* rp, packer* cp) { call_proxy(func, instance, rp, cp); }
    
    template <typename R, typename C, typename I, typename S>
    void call_proxy(R (C::*func)(I&), S& instance, packer* rp, packer* cp) {
        I* arg = cp->getv<I>();
        R result = (instance.*func)(*arg);

		response_t<R> response;
		response.set_code(RPC_SUCCESS);
		response.set_value(result);
		(*rp) << response;

    }
 
    void register_service(servicer_base const& service_instance) {
        std::apply(
            [this, &service_instance] (const auto&... method) {
                (register_method(
                    std::get<METHOD_NAME_IDX>(method), std::get<FUNC_PTR_IDX>(method), service_instance), ...);
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
            if ((accepted_fd = accept(listening_fd, 
                            reinterpret_cast<struct sockaddr*>(&client_addr), &addr_size)) < 0) {
                fprintf(stderr, "srpc::server::start(): accept failed.\n");
                continue;
            }

            std::vector<uint8_t> bytes = transport::recv_data(accepted_fd);  

            // deserialize the method name and service name        
            packer::ptr p = std::make_shared<packer>(std::move(bytes));
            std::string funcname;
            (*p) >> funcname;

            // call the service's method
            packer::ptr response = call(funcname, p);
        }

        close(listening_fd);
    }

private:
    std::unordered_map<std::string, std::function<void(packer*, packer*)>> _function_registry; 
};

} //namespace srpc
