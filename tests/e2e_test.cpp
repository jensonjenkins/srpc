#include <srpc/lexer.hpp>
#include <srpc/parser.hpp>
#include <srpc/generator.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>

namespace srpc {

TEST_CASE("end to end test") {
    // USER FLOW: 
    //
    // define proto file, 
    //
    // generate struct and stubs
    //  - lex, parse, generate
    // 
    // client side: 
    //   call method:
    //     - init struct
    //     - serialize struct to vector<u8>
    //     - send over
    //          -- wait for response (work done on server) -- 
    //     - recv vector<u8>
    //     - unpack vector<u8> (reconstruct the response struct)
    //     - return response
    //
    //   header file flow:
    //     generate stubs and structs:
    //     - lexer.hpp
    //     - parse.hpp
    //     - generate.hpp
    //     usage:
    //     - packer.hpp (struct to vec<u8>)
    //     - transport.hpp (send vec<u8>, await response vec<u8>)
    //     - packer.hpp (unpack vec<u8> to struct)
    //  
    //   example code:
    //     my_service_stub stub(); // registration of messages done here?
    //     stub.register_insecure_channel("127.0.0.1", "8080");
    //     request req{};
    //     response res = stub.some_method(req);
    // 
    //
    // server side:
    // RpcServer server();
    // server.register_service(MyService())
    // server.start("0.0.0.0", 5000)
}

}
