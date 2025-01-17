#pragma once

#include "element.hpp"
#include <fstream>
#include <sstream>

namespace srpc {

struct generator {

    static void handle_contract(const std::string& path) noexcept {
        for (const auto& e : contract::elements) {
            if (auto msg = dynamic_pointer_cast<message>(e)) {
                write_to_file(path, handle_message(msg));
            } else if (auto svc = dynamic_pointer_cast<service>(e)) {
                write_to_file(path, handle_service(svc));
            }
        }
    }

    [[nodiscard]] static std::string handle_service(std::shared_ptr<service> svc) noexcept {
        // Stub 
        std::ostringstream stub_stream, servicer_stream;

        stub_stream << "struct " << svc->name << "_stub {\n";
        stub_stream << "\t" << svc->name << "_stub() {\n";
        stub_stream << "\t\tif (!_init) {\n";

        for (const auto& md : svc->msg_dependencies()) {
            stub_stream << "\t\t\tsrpc::message_registry[\"" << md << "\"] = []() -> std::unique_ptr<" << md << "> {\n";
            stub_stream << "\t\t\t\treturn std::make_unique<" << md << ">();\n";
            stub_stream << "\t\t\t};\n";
        }

        stub_stream << "\t\t}\n";
        stub_stream << "\t\t_init = true;\n";
        stub_stream << "\t}\n\n";

        stub_stream << "\tvoid register_insecure_channel(std::string server_ip, std::string port) {\n";
        stub_stream << "\t\tif (_socket != -1) { close(_socket); }\n";
        stub_stream << "\t\t_socket = srpc::transport::create_client_socket(server_ip, port);\n";
        stub_stream << "\t}\n\n";

        for (const auto& m : svc->methods()) {
            stub_stream << get_client_stub_method(svc->name, m.get());
        }

        stub_stream << "private:\n\tstatic bool _init;\n";
        stub_stream << "\tint32_t _socket = -1;\n";
        stub_stream << "};\n\n";
        stub_stream << "inline bool " << svc->name << "_stub::_init = false;\n\n";

        // Generate Servicer
        servicer_stream << "struct " << svc->name << "_servicer : srpc::servicer_base {\n";

        for (const auto& m : svc->methods()) {
            servicer_stream << "\tvirtual " << m->output_t << " " << m->name << "(" << m->input_t << "& req)";
            servicer_stream << " { throw std::runtime_error(\"Method not implemented!\"); }\n";
        }
        servicer_stream << "\n";
        servicer_stream << "\tstatic constexpr const char* name = \"" << svc->name << "\";\n";
        servicer_stream << "\tstatic constexpr auto methods = std::make_tuple(\n";

        for (size_t i = 0; i < svc->methods().size(); i++) {
            servicer_stream << "\t\tSTRUCT_MEMBER(" << svc->name << "_servicer, " << svc->methods()[i]->name << ", \"" 
                << svc->name << "_servicer::" << svc->methods()[i]->name<< "\")";
            if (i != svc->methods().size() - 1) {
                servicer_stream << ",\n";
            }
        }
        servicer_stream << "\n\t);\n";
        servicer_stream << "};\n\n";

        return stub_stream.str() + servicer_stream.str();
    }

    [[nodiscard]] static std::string get_client_stub_method(const std::string& svc_name, method* m) noexcept {
        std::ostringstream msg_stream;

        msg_stream << "\t" << m->output_t << " " << m->name << "(" << m->input_t << "& req) {\n";

        msg_stream << "\t\tsrpc::packer pr;\n";
        msg_stream << "\t\tsrpc::request_t<" << m->input_t << "> request;\n";
        msg_stream << "\t\trequest.set_method_name(\"" << svc_name << "_servicer" << "::" << m->name << "\");\n";
        msg_stream << "\t\trequest.set_value(std::move(req));\n";
        msg_stream << "\t\tpr.pack_request(request);\n\n";

        msg_stream << "\t\tsrpc::transport::send_data(_socket, (*pr.buf()).data(), pr.size());\n"; 
        msg_stream << "\t\tsrpc::message_t res = srpc::transport::recv_data(_socket);\n";
        msg_stream << "\t\tsrpc::packer rpr(res.data(), res.size());\n\n";
        
        msg_stream << "\t\tsrpc::response_t<" << m->output_t << "> msg = rpr.unpack_response<" << m->output_t << ">();\n\n";
        msg_stream << "\t\treturn msg.value();\n";

        msg_stream << "\t}\n";

        return msg_stream.str();
    }

    [[nodiscard]] static std::string handle_message(std::shared_ptr<message> msg) noexcept {
        std::ostringstream msg_stream;
        msg_stream << "struct " <<  msg->name << " : public srpc::message_base {\n";
        for (const auto& fd : msg->fields()) {
            msg_stream << "\t" << fd->type << " " << fd->name << ";\n";
        }

        msg_stream << "\n\t// overrides\n";
        msg_stream << "\tstatic constexpr const char* name = \"" << msg->name << "\";\n";
        msg_stream << "\tstatic constexpr auto fields = std::make_tuple(\n";

        for (size_t i = 0; i < msg->fields().size(); i++) {
            msg_stream << "\t\tSTRUCT_MEMBER(" << msg->name << ", " << msg->fields()[i]->name << ", \"" 
                << msg->name << "::" << msg->fields()[i]->name<< "\")";
            if (i != msg->fields().size() - 1) {
                msg_stream << ",\n";
            }
        }
        msg_stream << "\n\t);\n";

        msg_stream << "\tvoid unpack(srpc::buffer::ptr bp) override {\n";
        msg_stream << "\t\tsrpc::packer p(bp);\n";

        for (const auto& fd : msg->fields()) {
            if (fd->is_primitive) {
                msg_stream << "\t\tp >> " << fd->name << ";\n";
            } else {
                msg_stream << "\t\t" << fd->name << " = *(p->getv());\n";
            }
        }
        msg_stream << "\t}\n";
        msg_stream << "};\n\n";

        return msg_stream.str();
    }
 
    static signed write_to_file(const std::string& file_path, const std::string& s) noexcept {
        std::ofstream file(file_path, std::ios::app);
        if (!file) { return 1; }
        file << s;
        file.close();
        return 0;
    }

    static signed init_file(const std::string& file_path) noexcept {
        std::ofstream file(file_path, std::ios::trunc);
        if (!file) { return 1; }
        file.close();

        std::ostringstream init_stream;
        init_stream << "#include <srpc/core.hpp>\n";
        init_stream << "#include <srpc/transport.hpp>\n";
        init_stream << "#include <srpc/packer.hpp>\n";
        init_stream << "#include <stdexcept>\n";
        init_stream << "#include <cstdint>\n\n";
        init_stream << "/**\n * This is an auto-generated file generated by srpc. Do not modify!\n */\n\n";

        return write_to_file(file_path, init_stream.str());
    }
};

} // namespace srpc
