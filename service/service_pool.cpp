#include "service_pool.h"
#include <sys/uio.h>
#include <sys/eventfd.h>
#include "sockets.h"
#include "loop.h"
#include "logging.h"

namespace smart {

ServicePool& ServicePool::get_instance()
{
    static ServicePool pool;
    return pool;
}

ServProc::ServProc(int fd, MPMCQueue<Letter>& letters):
    LoopThread("service_processor"),
    _queue_read_io(new IO(fd, EV_READ, false)),
    _letters(&letters)
{
}

bool ServProc::prepare()
{
    _queue_read_io->on_read([this]() {
        eventfd_t num = 0;
        eventfd_read(_queue_read_io->fd(), &num);
        for (auto i = 0; i < num; ++i) {
            Letter letter;
            if (!_letters->pop(&letter)) {
                break;
            }

            SLOG(INFO) << "begin to parse:" << letter.second->content();
            json request;
            json response;
            if (!letter.second->content().empty()) {
                request = json::parse(letter.second->content());
            }
            SLOG(INFO) << "begin to call method" << letter.second->get_service_name()
                       << " " << letter.second->get_method_name();
            auto status = ServicePool::get_instance()
                .find_service(letter.second->get_service_name())
                ->find_method(letter.second->get_method_name())(request, response);

            SLOG(INFO) << "content:" << response;
            letter.second->set_response(letter.first->o_buffer, status, response.dump());
            letter.first->o_buffer.cut_into_fd(letter.first->io->fd(), 0);
        }
    });

    return get_local_loop()->add_io(_queue_read_io);
}


Service::Method Service::_default_method = 
    [](const json&, json& response) -> HttpStatus {
        response["status"] = "fail";
        response["message"] = "method not found";

        return HttpStatus::BAD_REQUEST;
    };

bool ServicePool::run()
{
    _queue_write_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (_queue_write_fd < 0) {
        SLOG(WARNING) << "get eventfd fail!";
        return false;
    }

    for(int i = 0; i < std::thread::hardware_concurrency(); ++i) {
        _proc_pool.emplace_back(new ServProc(_queue_write_fd, _letters));
        if (!_proc_pool.back()->run()) {
            return false;
        }
    }

    return true;
}

void ServicePool::stop()
{
    for (auto &proc : _proc_pool) {
        proc->stop();
    }
    rpc::close(_queue_write_fd);
}

void ServicePool::send(Letter&& letter)
{
    _letters.push(letter);
    eventfd_write(_queue_write_fd, 1);
}

}

