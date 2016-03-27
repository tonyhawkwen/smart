#ifndef CORE_LOOP_H
#define CORE_LOOP_H

#include <thread>
#include <memory>
#include <vector>
#include <queue>
#include <ev.h>

namespace smart {

class IO;
class Loop;

struct smart_ev_io {
    struct ev_io eio;
    std::weak_ptr<IO> sio;
};

std::shared_ptr<Loop> get_local_loop();

class Loop : public std::enable_shared_from_this<Loop> {
public:
    Loop();
    virtual ~Loop();

    bool init();
    void loop();
    void quit();
    void pending();
    void resume();
    bool is_going_to_quit() { return _quit; }

    bool add_io(std::shared_ptr<IO>&);
    bool remove_io(std::shared_ptr<IO>&);
    bool stop_io(std::shared_ptr<IO>&);
    bool restart_io(std::shared_ptr<IO>&);

private:
    static void ev_io_common_cb(struct ev_loop* loop, struct ev_io* w, int revents);
    static void ev_async_cb(struct ev_loop* loop, struct ev_async* w, int revents);

    std::thread::id _owner;
    std::shared_ptr<IO> _wakeup;
    struct ev_loop* _loop;
    ev_async _async;
    bool _quit;
    std::vector<smart_ev_io> _ios;   
    std::queue<size_t> _idle_io_index;
};

}

#endif

