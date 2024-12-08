#include <limits>
#include <srpc/serializer.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>

namespace srpc {

TEST_CASE("Serializing primitive types", "[serialize][primitive]") {
    SECTION("Message: single int8_t") {
        int8_t arg1 = 5;
        std::string name = "i8";
        std::vector<uint8_t> res = serializer::serialize(name, arg1);
        std::vector<uint8_t> serialized {2, 0, 0, 0, 0, 0, 0, 0, 'i', '8', 5}; // big endian

        CAPTURE(name, arg1, res); 
        
        REQUIRE(serialized == res);
    }

    SECTION("Message: multiple primitives") {
        int8_t arg1 = 5;
        int32_t arg2 = std::numeric_limits<int32_t>::max();
        std::string arg3 = "arg3";

        std::string name = "i8_i32_str";
        std::vector<uint8_t> res = serializer::serialize(name, arg1, arg2, arg3);
        std::vector<uint8_t> serialized {
            10, 0, 0, 0, 0, 0, 0, 0, 
            'i', '8', '_', 'i', '3', '2', '_', 's', 't', 'r', 
            5, 
            255, 255, 255, 127, 
            4, 0, 0, 0, 0, 0, 0, 0,
            'a', 'r', 'g', '3'
        };
        CAPTURE(name, arg1, arg2, arg3, res); 

        REQUIRE(serialized == res);
    }

    SECTION("Service") {

    }
}

TEST_CASE("Serializing nested types", "[serialize][nested]") {
    SECTION("Message") {

    }

    SECTION("Service") {

    }

}


} // namespace srpc
