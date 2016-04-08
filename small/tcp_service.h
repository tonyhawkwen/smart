#ifndef SERVICE_TCP_SERVICE_H
#define SERVICE_TCP_SERVICE_H

#include <map>
#include "loop_thread.h"
#include "service_data.h"
#include "service.h"
#include "ssl.h"

namespace smart {

class TcpService : public LoopThread {
public:
    using EventCallback = std::function<void()>;

    TcpService():
        LoopThread("tcp_service"),
        _port(1080),
        _listen_io(new IO) {
            add_service(&_base_service, "Service");
        }
    
    TcpService(unsigned short port):
        LoopThread("tcp_service"),
        _port(port),
        _listen_io(new IO) {
            add_service(&_base_service, "Service");
        }

    virtual ~TcpService() {}

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

protected:
    bool prepare() override;
    void process_end() override;

private:
    unsigned short _port;
    std::shared_ptr<IO> _listen_io;
    int _idle_fd;
    std::map<int, SConnection> _conn_map; 
    Service _base_service;
    std::unordered_map<std::string, Service*> _service_map;
    std::shared_ptr<SSL_CTX> _ssl_ctx;
};

}

#endif /*SERVICE_TCP_SERVICE_H*/
