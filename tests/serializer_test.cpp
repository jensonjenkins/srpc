#include <cstdint>
#include <limits>
#include <srpc/core.hpp>
#include <srpc/serializer.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>

namespace srpc {

TEST_CASE("serializing primitive types", "[serialize][primitive]") {
    SECTION("single primitive") {
        int8_t arg1 = 5;
        std::string name = "i8";

        std::vector<uint8_t> res = serializer::serialize(name, arg1);
        std::vector<uint8_t> serialized {2, 0, 0, 0, 0, 0, 0, 0, 'i', '8', 5}; // big endian

        CAPTURE(name, arg1, res);         
        REQUIRE(serialized == res);
    }

    SECTION("multiple primitives") {
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
}

struct single_primitive : public message_base { 
    int8_t arg1;

    // statics
    static constexpr const char* name = "single_primitive";
    static constexpr auto fields = std::make_tuple(
        MESSAGE_FIELD(single_primitive, arg1)
    );
};

struct multiple_primitives: public message_base { 
    int8_t arg1;
    char arg2;
    int64_t arg3;
    std::string arg4;

    // statics
    static constexpr const char* name = "multiple_primitives";
    static constexpr auto fields = std::make_tuple(
        MESSAGE_FIELD(multiple_primitives, arg1),
        MESSAGE_FIELD(multiple_primitives, arg2),
        MESSAGE_FIELD(multiple_primitives, arg3),
        MESSAGE_FIELD(multiple_primitives, arg4)
    );
};

struct nested_message : public message_base {
    int64_t arg1;
    single_primitive arg2;
    multiple_primitives arg3;

    // statics
    static constexpr const char* name = "nested_message";
    static constexpr auto fields = std::make_tuple(
        MESSAGE_FIELD(nested_message, arg1),
        MESSAGE_FIELD(nested_message, arg2),
        MESSAGE_FIELD(nested_message, arg3)
    );
};

TEST_CASE("serializing structs", "[serialize][struct]") {
    SECTION("single primitive") {
        single_primitive sp;
        sp.arg1 = 5;

        std::vector<uint8_t> res = serializer::serialize(sp);
        std::vector<uint8_t> serialized {
            16, 0, 0, 0, 0, 0, 0, 0, 
            's', 'i', 'n', 'g', 'l', 'e', '_', 'p', 'r', 'i', 'm', 'i', 't', 'i', 'v', 'e',
            5, 
        };
        CAPTURE(res);
        REQUIRE(serialized == res);
    }

    SECTION("multiple primitives") {
        multiple_primitives mp;
        mp.arg1 = 22;
        mp.arg2 = 'z';
        mp.arg3 = std::numeric_limits<int64_t>::max();
        mp.arg4 = "testing_string";

        std::vector<uint8_t> res = serializer::serialize(mp);
        std::vector<uint8_t> serialized {
            19, 0, 0, 0, 0, 0, 0, 0, 
            'm', 'u', 'l', 't', 'i', 'p', 'l', 'e', '_', 'p', 'r', 'i', 'm', 'i', 't', 'i', 'v', 'e', 's',
            22,
            'z',
            255, 255, 255, 255, 255, 255, 255, 127, 
            14, 0, 0, 0, 0, 0, 0, 0,
            't', 'e', 's', 't', 'i', 'n', 'g', '_', 's', 't', 'r', 'i', 'n', 'g'
        };
        CAPTURE(res);
        REQUIRE(serialized == res);
    }

    SECTION("nested message") {
        single_primitive sp;
        sp.arg1 = 5;

        multiple_primitives mp;
        mp.arg1 = 22;
        mp.arg2 = 'z';
        mp.arg3 = std::numeric_limits<int64_t>::max();
        mp.arg4 = "testing_string";

        nested_message nm;
        nm.arg1 = std::numeric_limits<int64_t>::max();
        nm.arg2 = sp;
        nm.arg3 = mp;

        std::vector<uint8_t> res = serializer::serialize(nm);
        std::vector<uint8_t> serialized {
            14, 0, 0, 0, 0, 0, 0, 0, 
            'n', 'e', 's', 't', 'e', 'd', '_', 'm', 'e', 's', 's', 'a', 'g', 'e',
            255, 255, 255, 255, 255, 255, 255, 127, 
            16, 0, 0, 0, 0, 0, 0, 0, 
            's', 'i', 'n', 'g', 'l', 'e', '_', 'p', 'r', 'i', 'm', 'i', 't', 'i', 'v', 'e',
            5, 
            19, 0, 0, 0, 0, 0, 0, 0, 
            'm', 'u', 'l', 't', 'i', 'p', 'l', 'e', '_', 'p', 'r', 'i', 'm', 'i', 't', 'i', 'v', 'e', 's',
            22,
            'z',
            255, 255, 255, 255, 255, 255, 255, 127, 
            14, 0, 0, 0, 0, 0, 0, 0,
            't', 'e', 's', 't', 'i', 'n', 'g', '_', 's', 't', 'r', 'i', 'n', 'g'
        };
        CAPTURE(res);
        REQUIRE(serialized == res);
    }
}


} // namespace srpc
