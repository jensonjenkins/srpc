#pragma once 

#include "core.hpp"
#include "transport.hpp"
#include "packer.hpp"
#include <functional>
#include <type_traits>
#include <unordered_map>

namespace srpc {

class server {
public:
    server() = default;
    ~server() = default;

    packer::ptr call(std::string const& funcname, packer::ptr p) {
        packer::ptr rp = std::make_shared<packer>(); // packer to populate with return value

        auto it = _function_registry.find(funcname);
        if (it == _function_registry.end()) {
            fprintf(stderr, "srpc::server::call(): function %s not registered.\n", funcname.c_str());
            (*rp) << static_cast<uint8_t>(RPC_ERR_FUNCTION_NOT_REGISTERED);
        }

        auto func = it->second;
        func(rp.get(), p.get());
            
        return rp;
    }
    
    /// @tparam S                   (derived from servicer_base) servicer class
    /// @param  service_instance    instance of S
    template <SrpcService S>
    void register_service(S& service_instance) {
        static_assert(std::tuple_size_v<decltype(S::methods)> > 0, "S::methods is empty!");
        std::apply(
            [this, &service_instance] (const auto&... method) {
                (register_method(std::get<MEMBER_NAME>(method), std::get<MEMBER_ADDR>(method), service_instance), ...);
            },
            S::methods
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

            message_t msg = transport::recv_data(accepted_fd);  

            // deserialize the method name and service name        
            packer::ptr p = std::make_shared<packer>(msg.data(), msg.size());
            std::string funcname;
            (*p) >> funcname;

            // call the service's method
            packer::ptr r = call(funcname, p);
            assert(r->offset() == 0);

            transport::send_data(accepted_fd, r->data(), r->size());

            close(accepted_fd);
        }
        close(listening_fd);
    }

    void __testable_start(std::string const&&);

private:

    /// @brief
    /// @tparam F
    /// @tparam S
    template <typename F, SrpcService S> 
    void register_method(std::string const& name, F func, S& instance) {
        using input_type = typename function_traits<F>::input_type;
        using return_type = typename function_traits<F>::return_type;
        static_assert(std::is_base_of_v<message_base, std::decay_t<input_type>>);
        static_assert(std::is_base_of_v<message_base, std::decay_t<return_type>>);
 
        _function_registry[name] = std::bind(
                &server::call_proxy<F, S>, this, func, instance, std::placeholders::_1, std::placeholders::_2);
    }
  
    /// @tparam S           servicer class
    /// @tparam F           member function of S
    /// @param  func        member function pointer of type F
    /// @param  instance    instance of S
    /// @param  rp          packer to be populated with return value (return packer)
    /// @param  cp          packer containing data to be read from (client packer)
    template <typename F, SrpcService S>
    void call_proxy(F func, S& instance, packer* rp, packer* cp) { call_proxy_impl(func, instance, rp, cp); }
    
    
    /// @tparam R function return type 
    /// @tparam I function input type
    template <SrpcMessage R, typename C, SrpcMessage I, SrpcService S>
    void call_proxy_impl(R (C::*func)(I&), S& instance, packer* rp, packer* cp) {
        I* arg = cp->getv<I>();
        R result = (instance.*func)(*arg); // function call not a cast

        response_t<R> response;
        response.set_code(RPC_SUCCESS);
        response.set_value(result);
        rp->pack_response(response);
    }

    std::unordered_map<std::string, std::function<void(packer*, packer*)>> _function_registry; 
};

} //namespace srpc
