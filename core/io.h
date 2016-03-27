#ifndef CORE_IO_H
#define CORE_IO_H

#include <functional>

namespace smart {

class Loop;

class IO {
public:
    friend class Loop;
    using EventCallback = std::function<void()>;

    IO() : _fd(-1), _index(-1), _events(0), _revents(0) {}
    IO(int fd, int events) : _fd(fd), _index(-1), _events(events), _revents(0) {}
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
    EventCallback _read_func;
    EventCallback _write_func;
};

}
#endif /*CORE_IO_H*/
