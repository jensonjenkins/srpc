#include <cstdint>
#include <limits>
#include <srpc/core.hpp>
#include <srpc/packer.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>

namespace srpc {

struct single_primitive : public message_base { 
    int8_t arg1;

    // overrides 
    static constexpr const char* name = "single_primitive";
    static constexpr auto fields = std::make_tuple(
        MESSAGE_FIELD(single_primitive, arg1)
    );

    constexpr bool operator==(const single_primitive& other) const noexcept { return arg1 == other.arg1; }
    void unpack(const std::vector<uint8_t>& packed, size_t& offset) override {
        std::memcpy(&arg1, packed.data() + offset, sizeof(int8_t)); 
        offset += sizeof(int8_t);
    }
};

struct multiple_primitives : public message_base { 
    int8_t arg1;
    char arg2;
    int64_t arg3;
    std::string arg4;

    // overrides 
    static constexpr const char* name = "multiple_primitives";
    static constexpr auto fields = std::make_tuple(
        MESSAGE_FIELD(multiple_primitives, arg1),
        MESSAGE_FIELD(multiple_primitives, arg2),
        MESSAGE_FIELD(multiple_primitives, arg3),
        MESSAGE_FIELD(multiple_primitives, arg4)
    );

    constexpr bool operator==(const multiple_primitives& other) const noexcept { 
        return arg1 == other.arg1 &&
            arg2 == other.arg2 &&
            arg3 == other.arg3 &&
            arg4 == other.arg4; 
    }

    void unpack(const std::vector<uint8_t>& packed, size_t& offset) override {
        int64_t header_length = 0;
        std::memcpy(&arg1, packed.data() + offset, sizeof(int8_t)); 
        offset += sizeof(int8_t);
        std::memcpy(&arg2, packed.data() + offset, sizeof(char)); 
        offset += sizeof(char);
        std::memcpy(&arg3, packed.data() + offset, sizeof(int64_t)); 
        offset += sizeof(int64_t);


        std::memcpy(&header_length, packed.data() + offset, sizeof(int64_t));
        offset += sizeof(int64_t);
        arg4 = std::string(reinterpret_cast<const char*>(packed.data() + offset), header_length);
        offset += header_length;        
    }
};

struct nested_message : public message_base {
    int64_t arg1;
    single_primitive arg2;
    multiple_primitives arg3;

    // overrides
    static constexpr const char* name = "nested_message";
    static constexpr auto fields = std::make_tuple(
        MESSAGE_FIELD(nested_message, arg1),
        MESSAGE_FIELD(nested_message, arg2),
        MESSAGE_FIELD(nested_message, arg3)
    );

    constexpr bool operator==(const nested_message& other) const noexcept { 
        return arg1 == other.arg1 &&
            arg2 == other.arg2 &&
            arg3 == other.arg3;
    }

    void unpack(const std::vector<uint8_t>& packed, size_t& offset) override {
        int64_t header_length = 0;
        std::memcpy(&arg1, packed.data() + offset, sizeof(int64_t)); 
        offset += sizeof(int64_t);
        
        single_primitive arg2_;
        arg2_.unpack(packed, offset);
        arg2 = std::move(arg2_);

        multiple_primitives arg3_;
        arg3_.unpack(packed, offset);
        arg3 = std::move(arg3_);
    }
};

TEST_CASE("pack requests", "[pack][request]") {
    SECTION("single primitive") {
        single_primitive sp;
        sp.arg1 = 5;

        std::vector<uint8_t> res = packer::pack_request("test", sp);
        std::vector<uint8_t> packed {
            4, 0, 0, 0, 0, 0, 0, 0, 
            't', 'e', 's', 't',
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

        std::vector<uint8_t> res = packer::pack_request("test", mp);
        std::vector<uint8_t> packed {
            4, 0, 0, 0, 0, 0, 0, 0, 
            't', 'e', 's', 't',
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

        std::vector<uint8_t> res = packer::pack_request("test", nm);
        std::vector<uint8_t> packed {
            4, 0, 0, 0, 0, 0, 0, 0, 
            't', 'e', 's', 't',
            14, 0, 0, 0, 0, 0, 0, 0, 
            'n', 'e', 's', 't', 'e', 'd', '_', 'm', 'e', 's', 's', 'a', 'g', 'e',
            255, 255, 255, 255, 255, 255, 255, 127, 
            5, 
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

TEST_CASE("unpack request", "[unpack][request]") {
    message_registry["single_primitive"] = []() -> std::unique_ptr<single_primitive> { 
        return std::make_unique<single_primitive>(); 
    };
    message_registry["multiple_primitives"] = []() -> std::unique_ptr<multiple_primitives> { 
        return std::make_unique<multiple_primitives>(); 
    };
    message_registry["nested_message"] = []() -> std::unique_ptr<nested_message> { 
        return std::make_unique<nested_message>(); 
    };

    SECTION("single primitive") {
        single_primitive sp;
        sp.arg1 = 5;

        std::vector<uint8_t> packed {
            4, 0, 0, 0, 0, 0, 0, 0, 
            't', 'e', 's', 't',
            16, 0, 0, 0, 0, 0, 0, 0, 
            's', 'i', 'n', 'g', 'l', 'e', '_', 'p', 'r', 'i', 'm', 'i', 't', 'i', 'v', 'e',
            5, 
        };
        request_t<single_primitive> r = packer::unpack_request<single_primitive>(packed);
        REQUIRE(r.value() == sp); 
        REQUIRE(r.method_name() == "test");
    }

    SECTION("multiple primitives") {
        multiple_primitives mp;
        mp.arg1 = 22;
        mp.arg2 = 'z';
        mp.arg3 = std::numeric_limits<int64_t>::max();
        mp.arg4 = "testing_string";

        std::vector<uint8_t> packed {
            11, 0, 0, 0, 0, 0, 0, 0, 
            't', 'e', 's', 't', '_', 'm', 'e', 't', 'h', 'o', 'd',
            19, 0, 0, 0, 0, 0, 0, 0, 
            'm', 'u', 'l', 't', 'i', 'p', 'l', 'e', '_', 'p', 'r', 'i', 'm', 'i', 't', 'i', 'v', 'e', 's',
            22,
            'z',
            255, 255, 255, 255, 255, 255, 255, 127, 
            14, 0, 0, 0, 0, 0, 0, 0,
            't', 'e', 's', 't', 'i', 'n', 'g', '_', 's', 't', 'r', 'i', 'n', 'g'
        };
        request_t<multiple_primitives> r = packer::unpack_request<multiple_primitives>(packed);
        REQUIRE(r.value() == mp); 
        REQUIRE(r.method_name() == "test_method");
    }

    SECTION("nested messages") {
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

        std::vector<uint8_t> packed {
            4, 0, 0, 0, 0, 0, 0, 0, 
            't', 'e', 's', 't',
            14, 0, 0, 0, 0, 0, 0, 0, 
            'n', 'e', 's', 't', 'e', 'd', '_', 'm', 'e', 's', 's', 'a', 'g', 'e',
            255, 255, 255, 255, 255, 255, 255, 127, 
            5, 
            22,
            'z',
            255, 255, 255, 255, 255, 255, 255, 127, 
            14, 0, 0, 0, 0, 0, 0, 0,
            't', 'e', 's', 't', 'i', 'n', 'g', '_', 's', 't', 'r', 'i', 'n', 'g'
        };

        request_t<nested_message> r = packer::unpack_request<nested_message>(packed);
        REQUIRE(r.value() == nm); 
        REQUIRE(r.method_name() == "test");
    }
}

TEST_CASE("unpack response", "[unpack][response]") {
    message_registry["single_primitive"] = []() -> std::unique_ptr<single_primitive> { 
        return std::make_unique<single_primitive>(); 
    };
    message_registry["multiple_primitives"] = []() -> std::unique_ptr<multiple_primitives> { 
        return std::make_unique<multiple_primitives>(); 
    };
    message_registry["nested_message"] = []() -> std::unique_ptr<nested_message> { 
        return std::make_unique<nested_message>(); 
    };

    SECTION("single primitive") {
        single_primitive sp;
        sp.arg1 = 5;

        std::vector<uint8_t> packed {
            0,
            16, 0, 0, 0, 0, 0, 0, 0, 
            's', 'i', 'n', 'g', 'l', 'e', '_', 'p', 'r', 'i', 'm', 'i', 't', 'i', 'v', 'e',
            5, 
        };
        response_t<single_primitive> r = packer::unpack_response<single_primitive>(packed);
        REQUIRE(r.value() == sp); 
        REQUIRE(r.code() == RPC_SUCCESS);
    }

    SECTION("multiple primitives") {
        multiple_primitives mp;
        mp.arg1 = 22;
        mp.arg2 = 'z';
        mp.arg3 = std::numeric_limits<int64_t>::max();
        mp.arg4 = "testing_string";

        std::vector<uint8_t> packed {
            2,
            19, 0, 0, 0, 0, 0, 0, 0, 
            'm', 'u', 'l', 't', 'i', 'p', 'l', 'e', '_', 'p', 'r', 'i', 'm', 'i', 't', 'i', 'v', 'e', 's',
            22,
            'z',
            255, 255, 255, 255, 255, 255, 255, 127, 
            14, 0, 0, 0, 0, 0, 0, 0,
            't', 'e', 's', 't', 'i', 'n', 'g', '_', 's', 't', 'r', 'i', 'n', 'g'
        };
        response_t<multiple_primitives> r = packer::unpack_response<multiple_primitives>(packed);
        REQUIRE(r.value() == mp); 
        REQUIRE(r.code() == RPC_ERR_RECV_TIMEOUT);
    }

    SECTION("nested messages") {
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

        std::vector<uint8_t> packed {
            1,
            14, 0, 0, 0, 0, 0, 0, 0, 
            'n', 'e', 's', 't', 'e', 'd', '_', 'm', 'e', 's', 's', 'a', 'g', 'e',
            255, 255, 255, 255, 255, 255, 255, 127, 
            5, 
            22,
            'z',
            255, 255, 255, 255, 255, 255, 255, 127, 
            14, 0, 0, 0, 0, 0, 0, 0,
            't', 'e', 's', 't', 'i', 'n', 'g', '_', 's', 't', 'r', 'i', 'n', 'g'
        };

        response_t<nested_message> r = packer::unpack_response<nested_message>(packed);
        REQUIRE(r.value() == nm); 
        REQUIRE(r.code() == RPC_ERR_FUNCTION_NOT_REGISTERRED);
    }
}

} // namespace srpc
