#ifndef SERVICE_SERVICE_POOL_H
#define SERVICE_SERVICE_POOL_H

#include <unordered_map>
#include <vector>
#include "loop_thread.h"
#include "service_data.h"
#include "mpmc_queue.hpp"
#include "json.hpp"

using json = nlohmann::json;

namespace smart {

class ServProc : public LoopThread {
public:
    ServProc(int fd, MPMCQueue<Letter>& letters);
    virtual ~ServProc() {}

protected:
    bool prepare() override;

private:
    std::shared_ptr<IO> _queue_read_io;
    MPMCQueue<Letter>* _letters;
};

//FIXME: use protobuf in future
class Service {
public:
    using Method = std::function<HttpStatus(const json&, json&)>;
    Service() {
        _methods.reserve(24);//预留24个函数
    }
    template<typename S, typename T>
    void add_method(S&& name, T&& func) {
        _methods.emplace_back(func);
        _method_map.emplace(name, _methods.size() - 1);
    }

    Method& find_method(const std::string& name) {
        auto itr = _method_map.find(name);
        if (itr == _method_map.end()) {
            return _default_method;
        }

        return _methods[itr->second];
    }
    
    virtual void init() {
        add_method("Health", [](const json&, json& reply) -> HttpStatus {
            reply["status"] = "ok";
            return HttpStatus::OK;
        });
    }

private:
    static Method _default_method;
    std::vector<Method> _methods;
    std::unordered_map<std::string, size_t> _method_map;
};

class ServicePool {
public:
    ~ServicePool() {}
    static ServicePool& get_instance();

    bool run();
    void stop();
    void send(Letter&&);
    void add_service(Service* service, const std::string& name) {
        service->init();
        _service_map.emplace(name, service);
    }
    Service* find_service(const std::string& name) {
        auto itr = _service_map.find(name);
        if (itr == _service_map.end()) {
            return &_base_service;
        }

        return itr->second;
    }
    
private:
    ServicePool() {
        add_service(&_base_service, "Service");
    }
    
    int _queue_write_fd;   
    std::vector<std::unique_ptr<ServProc>> _proc_pool;
    MPMCQueue<Letter> _letters;
    Service _base_service;
    std::unordered_map<std::string, Service*> _service_map;
};

}

#endif /*SERVICE_SERVICE_POOL_H*/
