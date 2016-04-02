#ifndef SERVICE_SERVICE_POOL_H
#define SERVICE_SERVICE_POOL_H

#include "loop_thread.h"
#include "mpmc_queue.hpp"
#include "service.h"

namespace smart {

class ServProc : public LoopThread {
public:
    ServProc(int fd, MPMCQueue<Letter>& letters);
    virtual ~ServProc() {}

protected:
    bool prepare() override;
    void process_end() override;

private:
    std::shared_ptr<IO> _queue_read_io;
    MPMCQueue<Letter>* _letters;
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
