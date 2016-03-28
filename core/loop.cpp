#include "loop.h"
#include "define.h"
#include "logging.h"
#include "exception.h"
#include "io.h"

namespace smart {

std::shared_ptr<Loop> get_local_loop()
{
    static thread_local std::shared_ptr<Loop> s_local_loop;
    if (!s_local_loop) {
        s_local_loop.reset(new Loop);
    }

    return s_local_loop;
}

void IO::handle_event(int revents)
{
    if (revents & EV_READ) {
        _read_func();
    }

    if (revents & EV_WRITE) {
        _write_func();
    }
}

Loop::Loop() :
    _owner(std::this_thread::get_id()),
    _loop(nullptr),
    _quit(false)
{

}

Loop::~Loop()
{
    if (std::this_thread::get_id() == _owner
            && nullptr != _loop) {

        SLOG(INFO) << "quit now!";
        ev_loop_destroy(_loop);
    }
}

void Loop::ev_io_common_cb(struct ev_loop* loop, struct ev_io* w, int revents)
{
    smart_ev_io* one_io = (smart_ev_io*)w;
    auto sio = one_io->sio.lock();
    if (sio) {
        sio->handle_event(revents);
    }
}

void Loop::ev_async_cb(struct ev_loop* loop, struct ev_async* w, int revents)
{
    SLOG(INFO) << "break loop";
    ev_break(loop);
}

bool Loop::init()
{
    SLOG(INFO) << "init loop begin...";
    if (_loop != nullptr) {
        SLOG(WARNING) << "loop has been inited, skip.";
        return true;
    }

    _loop = ev_loop_new(EVBACKEND_EPOLL | EVFLAG_NOENV);
    if (_loop == nullptr) {
        SLOG(FATAL) << "create loop fail!";
        throw_system_error("create loop fail");
    }

    ev_async_init(&_async, ev_async_cb);
    ev_async_start(_loop, &_async);

    return true;
}

void Loop::loop() 
{
    _quit = false;
    if (S_LIKELY(_loop != nullptr)) {
        ev_run(_loop);
    }
}

void Loop::pending()
{
    if (S_LIKELY(_loop != nullptr)) {
        ev_suspend(_loop);
    }
}

void Loop::resume()
{
    if (S_LIKELY(_loop != nullptr)) {
        ev_resume(_loop);
    }
}

bool Loop::add_io(std::shared_ptr<IO>& io)
{
    if (S_UNLIKELY(_loop == nullptr)) {
        return false;
    }
    if ((!io) || io->index() >= 0) {
        return true;
    }

    std::shared_ptr<smart_ev_io>* one_io;
    if (_idle_io_index.empty()) {
        _ios.emplace_back(new smart_ev_io());
        io->set_index(_ios.size() - 1);
        one_io = &_ios.back();
    } else {
        auto index = _idle_io_index.front();
        _idle_io_index.pop();
        io->set_index(index);
        one_io = &_ios[index];
    }

    (*one_io)->sio = io;
    ev_io_init(&(*one_io)->eio, ev_io_common_cb, io->fd(), io->events());
    ev_io_start(_loop, &(*one_io)->eio);

    return true;
}

bool Loop::remove_io(std::shared_ptr<IO>& io)
{
    if (S_UNLIKELY(_loop == nullptr)) {
        return false;
    }
    
    if ((!io) || io->index() < 0 || io->index() >= _ios.size()) {
        return true;
    }
    
    auto one_io = _ios[io->index()];
    if (S_UNLIKELY(one_io->sio.owner_before(io))) {
        SLOG(WARNING) << "ios'index[" << io->index() << "] has changed!";
        return false;
    }

    ev_io_stop(_loop, &one_io->eio);
    _idle_io_index.push(io->index());
    io->set_index(-1);
    one_io->sio.reset();

    return true;
}

bool Loop::stop_io(std::shared_ptr<IO>& io)
{
    if (S_UNLIKELY(_loop == nullptr)) {
        return false;
    }
    
    if ((!io) || io->index() < 0) {
        return true;
    }

    auto one_io = _ios[io->index()];
    ev_io_stop(_loop, &one_io->eio);
    return true;
}

bool Loop::restart_io(std::shared_ptr<IO>& io)
{
    if (S_UNLIKELY(_loop == nullptr)) {
        return false;
    }
    
    if ((!io) || io->index() < 0) {
        return true;
    }

    auto one_io = _ios[io->index()];
    ev_io_start(_loop, &one_io->eio);
    return true;
}

void Loop::quit()
{
    _quit = true;
    if (nullptr != _loop) {
        if (std::this_thread::get_id() == _owner) {
            ev_break(_loop);
        } else {
            SLOG(INFO) << "quit async!";
            ev_async_send(_loop, &_async);       
        }
    }   
}

}

