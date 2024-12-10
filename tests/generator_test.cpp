#include <srpc/lexer.hpp>
#include <srpc/parser.hpp>
#include <srpc/generator.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>

namespace srpc {

TEST_CASE("Generate Header File", "[generate][message]") {
    SECTION("Primitive Message") {
        contract::elements.clear();
        std::string input = R"(
            message Request {
                string arg1 = 1;
                int32 arg2 = 2;
                bool arg3 = 3;
            }
        )";
        lexer l(input);
        parser p(l); 
        p.parse_contract();
        REQUIRE(p.errors().size() == 0);

        auto msg = dynamic_pointer_cast<message>(contract::elements["Request"]);
        std::string res = generator::handle_message(msg);

        CAPTURE(res);
        CHECK(false);
    }
}

}
