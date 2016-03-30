#ifndef CORE_LOOP_THREAD_H
#define CORE_LOOP_THREAD_H

#include <string>
#include <thread>
#include <memory>
#include <mutex>
#include <future>

namespace smart {

class Loop;

class LoopThread {
public:
    LoopThread(const std::string& name) : 
        _name(name),
        _creator(std::this_thread::get_id()) {}

    virtual ~LoopThread() {}
    LoopThread(const LoopThread&) = delete;
    LoopThread& operator= (const LoopThread&) = delete;
    
    bool run();
    virtual void stop();

protected:
    virtual bool prepare() {return true;}
    virtual void process_end() {}

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

#endif /*CORE_LOOP_THREAD_H*/
