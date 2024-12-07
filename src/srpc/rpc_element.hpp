#pragma once

#include "token.hpp"
#include <cstddef>
#include <vector>
#include <typeindex>
#include <unordered_map>
#include <functional>

namespace srpc {

struct rpc_element {
    std::string name;

    virtual ~rpc_element() noexcept = default;
    virtual const std::string to_string() const noexcept = 0;
    virtual const std::string token_literal() const noexcept = 0;
};

class service : public rpc_element {
private:
    token                               _token;
    std::vector<std::function<void()>>  _methods;

public:
    service(token t) : _token(t) {}
    ~service() = default;

    const std::string to_string() const noexcept override { return "service"; }
    const std::string token_literal() const noexcept override { return _token.literal; }
};

struct field_descriptor {
    bool        is_optional;
    int         field_number;
    std::string name;
    std::string type;

    field_descriptor() {}
    field_descriptor(bool opt, int fn, std::string name, std::string t) 
        : is_optional(opt), field_number(fn), name(name), type(t) {};
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
    static void add_element(rpc_element* e) { elements.emplace(e->name, std::unique_ptr<rpc_element>(e)); }
    
    static std::unordered_map<std::string, std::unique_ptr<rpc_element>> elements;
};


} // namespace srpc
