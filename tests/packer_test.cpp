#include <cstdint>
#include <limits>
#include <srpc/core.hpp>
#include <srpc/packer.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>

namespace srpc {

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

        std::vector<uint8_t> res = packer::pack(sp);
        std::vector<uint8_t> packed {
            16, 0, 0, 0, 0, 0, 0, 0, 
            's', 'i', 'n', 'g', 'l', 'e', '_', 'p', 'r', 'i', 'm', 'i', 't', 'i', 'v', 'e',
            5, 
        };
        CAPTURE(res);
        REQUIRE(packed == res);
    }

    SECTION("multiple primitives") {
        multiple_primitives mp;
        mp.arg1 = 22;
        mp.arg2 = 'z';
        mp.arg3 = std::numeric_limits<int64_t>::max();
        mp.arg4 = "testing_string";

        std::vector<uint8_t> res = packer::pack(mp);
        std::vector<uint8_t> packed {
            19, 0, 0, 0, 0, 0, 0, 0, 
            'm', 'u', 'l', 't', 'i', 'p', 'l', 'e', '_', 'p', 'r', 'i', 'm', 'i', 't', 'i', 'v', 'e', 's',
            22,
            'z',
            255, 255, 255, 255, 255, 255, 255, 127, 
            14, 0, 0, 0, 0, 0, 0, 0,
            't', 'e', 's', 't', 'i', 'n', 'g', '_', 's', 't', 'r', 'i', 'n', 'g'
        };
        CAPTURE(res);
        REQUIRE(packed == res);
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

        std::vector<uint8_t> res = packer::pack(nm);
        std::vector<uint8_t> packed {
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
        REQUIRE(packed == res);
    }
}

} // namespace srpc
