#ifndef SERVICE_SERVICE_H
#define SERVICE_SERVICE_H

#include <unordered_map>
#include <vector>
#include <functional>
#include "service_data.h"
#include "json.hpp"

using json = nlohmann::json;
using shared_json = std::shared_ptr<json>;

namespace smart {

enum class ErrCode {
    OK = 200,
    PARAM_ERROR = 400,
    REGISTERED = 401,
    LOGIN_ERR = 402,
    SYSTEM_ERROR = 500
};

class Control {
public:
    Control(Letter& letter, shared_json response)
        : _conn(letter.first),
          _message(letter.second),
          _response(response),
          _status(HttpStatus::OK) {}
    ~Control() {
        _message->set_response(_conn->o_buffer, _status, _response->dump());
        _conn->o_buffer.cut_into_fd(_conn->io->fd(), 0);
    }

    void set_http_status(HttpStatus status) { _status = status; }

private:
    SConnection _conn;
    SHttpMessage _message;
    shared_json _response;
    HttpStatus _status;
};

//FIXME: use protobuf in future
class Service {
public:
    using SControl = std::shared_ptr<smart::Control>;
    using Method = std::function<void(const shared_json&, shared_json&, SControl&)>;
    Service() {
        _methods.reserve(24);//预留24个函数
    }
    template<typename S, typename T>
    void add_method(S&& name, T&& func) {
        _methods.emplace_back(std::forward<T>(func));
        _method_map.emplace(std::forward<S>(name), _methods.size() - 1);
    }

    Method& find_method(const std::string& name) {
        auto itr = _method_map.find(name);
        if (itr == _method_map.end()) {
            return _default_method;
        }

        return _methods[itr->second];
    }
    
    virtual void init() {
        add_method("Health", [](const shared_json&, 
                    shared_json& reply, SControl&) {
            (*reply)["status"] = "ok";
        });
    }

private:
    static Method _default_method;
    std::vector<Method> _methods;
    std::unordered_map<std::string, size_t> _method_map;
};

}

#endif /*SERVICE_SERVICE_H*/
