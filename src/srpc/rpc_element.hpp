#pragma once

#include "token.hpp"
#include <cstddef>
#include <vector>
#include <unordered_map>
#include <functional>

namespace srpc {

struct rpc_element {
    std::string name;

    virtual ~rpc_element() noexcept = default;
    virtual const std::string to_string() const noexcept = 0;
    virtual const std::string token_literal() const noexcept = 0;
};

struct method {
    std::string name;
    std::string input_t;
    std::string output_t;
    std::function<void(const void*, void*)> implementation;
    
    method() = default;
    method(std::string n, std::string in, std::string out, std::function<void(const void*, void*)> impl)
        : name(std::move(n)), input_t(in), output_t(out), implementation(std::move(impl)) {}
};

class service : public rpc_element {
private:
    token                                   _token;
    std::vector<std::unique_ptr<method>>    _methods;

public:
    service(token t) : _token(t) {}
    ~service() = default;

    void add_method(method* m) noexcept { _methods.push_back(std::unique_ptr<method>(m)); }

    std::vector<std::unique_ptr<method>>& methods() noexcept { return _methods; }

    const std::string to_string() const noexcept override { return "service"; }
    const std::string token_literal() const noexcept override { return _token.literal; }
};

struct field_descriptor {
    bool        is_optional;
    bool        is_primitive;
    int         field_number;
    std::string name;
    std::string type;

    field_descriptor() {}
    field_descriptor(bool opt, bool ip, int fn, std::string name, std::string t) 
        : is_optional(opt), is_primitive(ip), field_number(fn), name(name), type(t) {};
};

class message : public rpc_element {
private:
    token                                           _token;
    std::vector<std::unique_ptr<field_descriptor>>  _fields;

public:
    message(token t) : _token(t) {}
    ~message() = default;

    const std::string to_string() const noexcept override { return "message"; }
    const std::string token_literal() const noexcept override { return _token.literal; }
    
    void add_field_descriptor(field_descriptor* fd) noexcept { _fields.push_back(std::unique_ptr<field_descriptor>(fd)); }
    
    std::vector<std::unique_ptr<field_descriptor>>& fields() noexcept { return _fields; }
};

struct contract {
    contract() {}
    ~contract() = default;
    static void add_element(rpc_element* e) { elements.emplace(e->name, std::shared_ptr<rpc_element>(e)); }
    
    static std::unordered_map<std::string, std::shared_ptr<rpc_element>> elements;
};


} // namespace srpc
