#include <srpc/parser.hpp>
#include <srpc/generator.hpp>

#include <iostream>
#include <fstream>
#include <cassert>

namespace srpc {

size_t trace::indent_level = 0;
bool trace::enable_trace = 0;
std::vector<std::shared_ptr<rpc_element>> contract::elements;
std::unordered_map<std::string, size_t> contract::element_index_map;

void check_parser_errors(parser& p) {
    std::vector<std::string> parser_errors = p.errors();
    std::cout << "Found " << parser_errors.size() << " parser errors." << std::endl;
    for (std::string_view error : parser_errors) {
        std::cout << "parser error: " << error;
    }
    assert(parser_errors.size() == 0);
}

} // namespace srpc

std::string get_contract_dir(std::string contract_path) {
    size_t lidx = contract_path.find_last_of("/"); // probably a horrible idea to hardcode "/"                                   
    return contract_path.substr(0, lidx);
}

std::string get_filename(const std::string& contract_path) {
    size_t sidx = contract_path.find_last_of('/'); // probably a bad idea to hardcode "/"
    size_t lidx = contract_path.find_last_of('.'); 

    // edge cases 
    if (sidx == std::string::npos) {
        sidx = 0; // no directory separator; start from the beginning
    } else {
        sidx += 1; // skip the '/' itself
    }

    if (lidx == std::string::npos || lidx <= sidx) {
        lidx = contract_path.size(); // no file extension; take the full remainder
    }

    return contract_path.substr(sidx, lidx - sidx);
}

int main(int argc, char* argv[]) { 
    std::string contract_path(argv[1]);

    std::string filename = get_filename(contract_path);
    std::string directory = get_contract_dir(contract_path);

    std::cout << "Reading contract from: " << contract_path << std::endl;
    std::cout << "Contract name: " << filename << std::endl;
    std::cout << "Contract directory: " << directory << std::endl;

    std::ifstream ifs(contract_path);
    std::ostringstream oss;
    oss << ifs.rdbuf();
    std::string contract = oss.str();
    
    std::cout << "Parsing contract..." << std::endl;
    srpc::lexer l(contract);
    srpc::parser p(l);
    p.parse_contract();
    check_parser_errors(p);

    std::string generated_path = directory + "/" + filename + "_srpc.cpp";
    std::cout << "Generating stubs and services to: " << generated_path << std::endl;
    
    srpc::generator::init_file(generated_path);
    srpc::generator::handle_contract(generated_path);
    std::cout << "Completed generating stubs and services." << std::endl;

    return 0;
}

