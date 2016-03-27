#include "service.h"
#include <sstream>
#include "loop.h"
#include "exception.h"
#include "logging.h"

namespace smart {

void Service::stop()
{
    SLOG(INFO) << "begin to stop " << _name;
    if (std::this_thread::get_id() != _creator) {
        SLOG(WARNING) << "you have no access to destroy";
    }

    std::lock_guard<std::mutex> lock(_mutex);
    if (_loop) {
        _loop->quit();
    }
    if (_thread->joinable()) {
        _thread->join();
    }
}

bool Service::run()
{
    std::lock_guard<std::mutex> lock(_mutex);

    bool ret = false;

    try {
        std::future<bool> fut = _created.get_future();
        _thread.reset(new std::thread(&Service::process, this));
        ret = fut.get();
    } catch (const std::exception& ex) {
        std::stringstream sstream;
        sstream << "service: thread creation failed : " << ex.what();
        throw_system_error(sstream.str());
    } catch (...) {
        throw_system_error("service: thread creation failed : unknown reason");
    }

    return ret;
}

void Service::process()
{
    SLOG(INFO) << "service process init begin...";
    bool ret = false;
    try {
        do {
            if(!get_local_loop()->init()) {
                break;
            }
            
            ret = prepare();
        } while (0);

        _loop = get_local_loop();
        _created.set_value(ret);

        if (!ret) {
            SLOG(WARNING) << "service init fail!";
            return;
        }
    } catch (const std::exception& ex) {
        std::stringstream sstream;
        sstream << "thread creation failed : " << ex.what();
        _created.set_exception(std::make_exception_ptr(Exception(sstream.str())));
        return;
    } catch (...) {
        _created.set_exception(std::current_exception());
        return;
    }
    
    _self = std::this_thread::get_id();
    std::stringstream sstream;
    sstream << _name << "_" << _self;
    sstream >> _name;

    SLOG(INFO) << _name << " loop begin...";
    get_local_loop()->loop();
    SLOG(INFO) << _name << " loop end.";
}

}

