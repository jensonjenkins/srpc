#pragma once

#include "rpc_element.hpp"
#include <fstream>

namespace srpc {

struct generator {

    static void handle_contract() noexcept {
        for (const auto& pair : contract::elements) {
            if (auto msg = dynamic_pointer_cast<message>(pair.second)) {
                write_to_file(handle_message(msg));
            } else if (auto svc = dynamic_pointer_cast<service>(pair.second)) {
                write_to_file(handle_service(svc));
            }
        }
    }

    static std::string handle_message(std::shared_ptr<message> msg) noexcept {
        std::string msg_gen = "struct " + msg->name + " : public message_base {\n";
        for (const auto& fd : msg->fields()) {
            msg_gen += "\t" + fd->type + " " + fd->name + ";\n";
        }

        msg_gen += "\n\t// overrides\n";
        msg_gen += "\tstatic constexpr const char* name = \"" + msg->name + "\";\n";
        msg_gen += "\tstatic constexpr auto fields = std::make_tuple(\n";

        for (const auto& fd : msg->fields()) {
            msg_gen += "\t\tMESSAGE_FIELD(" + msg->name + ", " + fd->name + "),\n";
        }

        msg_gen.pop_back();
        msg_gen.pop_back();
        msg_gen += "\n\t);\n";

        msg_gen += "\tvoid unpack(const std::vector<uint8_t>& packed, size_t offset) override {\n";
        msg_gen += "\t\tint64_t header_length = 0;\n";

        for (const auto& fd : msg->fields()) {
            if (fd->is_primitive) {
                msg_gen += handle_primitive_field(fd->type, fd->name);
            } else {
                // TODO: Implement nested struct code generation
            }
        }

        msg_gen += "\t}\n";
        msg_gen += "};\n";

        return msg_gen;
    }

    [[nodiscard]] static std::string handle_primitive_field(const std::string& type, const std::string& name) noexcept {
        std::string msg_gen;
        if (type == "std::string") {
            msg_gen += "\t\tstd::memcpy(&header_length, packed.data() + offset, sizeof(int64_t));\n";
            msg_gen += "\t\toffset += sizeof(int64_t);\n"; 
            msg_gen += "\t\t" + name 
                + " = std::string(reinterpret_cast<const char*>(packed.data() + offset), header_length);\n";
            msg_gen += "\t\toffset += sizeof(header_length);\n"; 
        } else {
            msg_gen += "\t\tstd::memcpy(&" + name + ", packed.data() + offset, sizeof(" + type + "));\n";
            msg_gen += "\t\toffset += sizeof(" + type + ");\n";
        }
        return msg_gen;
    }

    static std::string handle_service(std::shared_ptr<service> svc) noexcept {
        std::string msg_gen;
        return msg_gen;
    }

    static signed write_to_file(const std::string& s) noexcept {
        std::ofstream file("msg_gen.hpp", std::ios::app);
        if (!file) {
            return 1;
        }
        file << s;
        file.close();
        return 0;
    }
};

} // namespace srpc
