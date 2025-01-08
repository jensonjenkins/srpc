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
    void unpack(buffer::ptr bp) override {
        packer p(bp);
        p >> arg1;
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

    void unpack(buffer::ptr bp) override {
        packer p(bp);
        p >> arg1;
        p >> arg2;
        p >> arg3;
        p >> arg4;
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

    void unpack(buffer::ptr bp) override {
        packer p(bp);
        p >> arg1;
        
        single_primitive arg2_;
        arg2_.unpack(bp);
        arg2 = std::move(arg2_);

        multiple_primitives arg3_;
        arg3_.unpack(bp);
        arg3 = std::move(arg3_);
    }
};

TEST_CASE("pack requests", "[pack][request]") {
    SECTION("single primitive") {
        single_primitive sp;
        sp.arg1 = 5;
        
        packer pr;
        request_t<single_primitive> req;
        req.set_value(std::move(sp));
        req.set_method_name("test");
        pr.pack_request(req);

        std::vector<uint8_t> packed {
            4, 0, 0, 0, 0, 0, 0, 0, 
            't', 'e', 's', 't',
            16, 0, 0, 0, 0, 0, 0, 0, 
            's', 'i', 'n', 'g', 'l', 'e', '_', 'p', 'r', 'i', 'm', 'i', 't', 'i', 'v', 'e',
            5, 
        };
        CAPTURE(*pr.buf());
        REQUIRE(packed == *pr.buf());
    }

    SECTION("multiple primitives") {
        multiple_primitives mp;
        mp.arg1 = 22;
        mp.arg2 = 'z';
        mp.arg3 = std::numeric_limits<int64_t>::max();
        mp.arg4 = "testing_string";

        packer pr;
        request_t<multiple_primitives> req;
        req.set_value(std::move(mp));
        req.set_method_name("test");
        pr.pack_request(req);

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
        CAPTURE(*pr.buf());
        REQUIRE(packed == *pr.buf());
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
        
        packer pr;
        request_t<nested_message> req;
        req.set_value(std::move(nm));
        req.set_method_name("test");
        pr.pack_request(req);

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
        CAPTURE(*pr.buf());
        REQUIRE(packed == *pr.buf());
    }
}

TEST_CASE("pack response", "[pack][response]") {
    SECTION("single primitive") {
        single_primitive sp;
        sp.arg1 = 5;

        packer pr;
        response_t<single_primitive> res;
        res.set_value(std::move(sp));
        res.set_code(RPC_SUCCESS);
        pr.pack_response(res);

        std::vector<uint8_t> packed {
            0,
            16, 0, 0, 0, 0, 0, 0, 0, 
            's', 'i', 'n', 'g', 'l', 'e', '_', 'p', 'r', 'i', 'm', 'i', 't', 'i', 'v', 'e',
            5, 
        };
        CAPTURE(*pr.buf());
        REQUIRE(packed == *pr.buf());
    }

    SECTION("multiple primitives") {
        multiple_primitives mp;
        mp.arg1 = 22;
        mp.arg2 = 'z';
        mp.arg3 = std::numeric_limits<int64_t>::max();
        mp.arg4 = "testing_string";
    
        packer pr;
        response_t<multiple_primitives> res;
        res.set_value(std::move(mp));
        res.set_code(RPC_ERR_RECV_TIMEOUT);
        pr.pack_response(res);

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
        CAPTURE(*pr.buf());
        REQUIRE(packed == *pr.buf());
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
        
        packer pr;
        response_t<nested_message> res;
        res.set_value(std::move(nm));
        res.set_code(RPC_ERR_FUNCTION_NOT_REGISTERRED);
        pr.pack_response(res);

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
        CAPTURE(*pr.buf());
        REQUIRE(packed == *pr.buf());
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

        std::vector<uint8_t> bytes {
            4, 0, 0, 0, 0, 0, 0, 0, 
            't', 'e', 's', 't',
            16, 0, 0, 0, 0, 0, 0, 0, 
            's', 'i', 'n', 'g', 'l', 'e', '_', 'p', 'r', 'i', 'm', 'i', 't', 'i', 'v', 'e',
            5, 
        };
        packer pr(bytes);
        request_t<single_primitive> r = pr.unpack_request<single_primitive>();
        REQUIRE(r.value() == sp); 
        REQUIRE(r.method_name() == "test");
    }

    SECTION("multiple primitives") {
        multiple_primitives mp;
        mp.arg1 = 22;
        mp.arg2 = 'z';
        mp.arg3 = std::numeric_limits<int64_t>::max();
        mp.arg4 = "testing_string";

        std::vector<uint8_t> bytes {
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
        packer pr(bytes);
        request_t<multiple_primitives> r = pr.unpack_request<multiple_primitives>();
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

        std::vector<uint8_t> bytes {
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

        packer pr(bytes);
        request_t<nested_message> r = pr.unpack_request<nested_message>();
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

        std::vector<uint8_t> bytes {
            0,
            16, 0, 0, 0, 0, 0, 0, 0, 
            's', 'i', 'n', 'g', 'l', 'e', '_', 'p', 'r', 'i', 'm', 'i', 't', 'i', 'v', 'e',
            5, 
        };
        packer pr(bytes);
        response_t<single_primitive> r = pr.unpack_response<single_primitive>();
        REQUIRE(r.value() == sp); 
        REQUIRE(r.code() == RPC_SUCCESS);
    }

    SECTION("multiple primitives") {
        multiple_primitives mp;
        mp.arg1 = 22;
        mp.arg2 = 'z';
        mp.arg3 = std::numeric_limits<int64_t>::max();
        mp.arg4 = "testing_string";

        std::vector<uint8_t> bytes {
            2,
            19, 0, 0, 0, 0, 0, 0, 0, 
            'm', 'u', 'l', 't', 'i', 'p', 'l', 'e', '_', 'p', 'r', 'i', 'm', 'i', 't', 'i', 'v', 'e', 's',
            22,
            'z',
            255, 255, 255, 255, 255, 255, 255, 127, 
            14, 0, 0, 0, 0, 0, 0, 0,
            't', 'e', 's', 't', 'i', 'n', 'g', '_', 's', 't', 'r', 'i', 'n', 'g'
        };
        packer pr(bytes);
        response_t<multiple_primitives> r = pr.unpack_response<multiple_primitives>();
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

        std::vector<uint8_t> bytes {
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
        packer pr(bytes);
        response_t<nested_message> r = pr.unpack_response<nested_message>();
        REQUIRE(r.value() == nm); 
        REQUIRE(r.code() == RPC_ERR_FUNCTION_NOT_REGISTERRED);
    }
}

} // namespace srpc
