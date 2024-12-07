#include <string>
#include <iostream>

namespace srpc {

struct trace {
    trace(std::string name) : name(name) {
        if(enable_trace) {
            std::cout<<std::string(indent_level * 4, ' ')<<"BEGIN "<<name<<std::endl;
            ++indent_level;
        }
    };
    ~trace() {
        if(enable_trace) {
            --indent_level;
            std::cout<<std::string(indent_level * 4, ' ')<<"END "<<name<<std::endl;
        }
    };

    std::string     name;
    static bool     enable_trace;
    static size_t   indent_level;
};

} // namespace parser

