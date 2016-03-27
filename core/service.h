#ifndef CORE_SERVICE_H
#define CORE_SERVICE_H

#include <string>
#include <thread>
#include <memory>
#include <mutex>
#include <future>

namespace smart {

class Loop;

class Service {
public:
    Service(const std::string& name) : 
        _name(name),
        _creator(std::this_thread::get_id()){}

    virtual ~Service() {}
    Service(const Service&) = delete;
    Service& operator= (const Service&) = delete;
    
    bool run();
    void stop();

protected:
    virtual bool prepare() {return true;}

private:
    void process();

    std::string _name;
    std::thread::id _creator;
    std::thread::id _self;
    std::unique_ptr<std::thread> _thread;
    std::promise<bool> _created;
    std::mutex _mutex;
    std::shared_ptr<Loop> _loop;
};

}

#endif /*CORE_SERVICE_H*/
