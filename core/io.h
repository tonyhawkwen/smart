#ifndef CORE_IO_H
#define CORE_IO_H

#include <functional>
#include "sockets.h"

namespace smart {

class Loop;

class IO {
public:
    friend class Loop;
    using EventCallback = std::function<void()>;

    IO() : _fd(-1), _index(-1), _events(0), _revents(0), _own_fd(true) {}
    IO(int fd, int events) : _fd(fd), _index(-1), _events(events), _revents(0), _own_fd(true) {}
    IO(int fd, int events, bool own_fd) : _fd(fd), _index(-1), _events(events), _revents(0), _own_fd(own_fd) {}
    ~IO() {
        if (_own_fd && _fd > 0) {
            rpc::close(_fd);
            _fd = -1;
        }
    }

    template<typename T>
    void on_read(T&& func) { _read_func = func; }
    template<typename T>
    void on_write(T&& func) { _write_func = func; }
    int fd() { return _fd; }
    void set_fd(int fd) { _fd = fd; }
    int events() { return _events; }
    void set_events(int events) { _events = events; }

private:
    void handle_event(int revents);
    int index() { return _index; }
    void set_index(int index) { _index = index; }

    int _fd;
    int _index;
    int _events;
    int _revents;
    bool _own_fd;
    EventCallback _read_func;
    EventCallback _write_func;
};

}
#endif /*CORE_IO_H*/
