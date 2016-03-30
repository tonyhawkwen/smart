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
    _queue_read_io(new IO(fd, EV_READ)),
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

            rapidjson::Document request;
            request.Parse(letter.second->content());
            rapidjson::Document response;
            response.SetObject();
            auto status = ServicePool::get_instance().find_service("xxx")
                ->find_method("yyy")(request, response);
            rapidjson::StringBuffer out;
            rapidjson::Writer<rapidjson::StringBuffer> writer(out);
            response.Accept(writer);

            SLOG(INFO) << "content:" << out.GetString();
            Buffer out_buffer;
            letter.second->set_response(out_buffer, status, out.GetString());
            out_buffer.cut_into_fd(letter.first->io->fd(), 0);
        }
    });

    return get_local_loop()->add_io(_queue_read_io);
}


Service::Method Service::_default_method = 
    [](const rapidjson::Document&, rapidjson::Document& response) -> HttpStatus {
        response.AddMember("status", "fail", response.GetAllocator());
        response.AddMember("message", "method not found", response.GetAllocator());

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

