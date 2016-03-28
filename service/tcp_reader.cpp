#include "tcp_reader.h"
#include <sys/uio.h>
#include <sys/eventfd.h>
#include "sockets.h"
#include "loop.h"
#include "logging.h"

namespace smart {

TcpReader::TcpReader(int fd, MPMCQueue<SConnection>& connections):
    Service("tcp_reader"),
    _queue_read_io(new IO(fd, EV_READ)),
    _connections(&connections)
{
}

TcpReader::~TcpReader()
{
}

bool TcpReader::prepare()
{
    _queue_read_io->on_read([this]() {
        eventfd_t num = 0;
        eventfd_read(_queue_read_io->fd(), &num);
        for (auto i = 0; i < num; ++i) {
            SConnection info;
            if (!_connections->pop(&info)) {
                break;
            }
            
            if (!info) {
                SLOG(WARNING) << "info empty!";
                continue;
            }

            while (1) {
                int error = 0;
                auto ret = info->buffer.append_from_fd(info->io->fd(), &error);
                if (ret == -2) {
                    SLOG(WARNING) << "buffer is full";
                    usleep(1000);
                } else if (ret == -1) {
                    if (error != EAGAIN) {
                        SLOG(WARNING) << "close connection, reason:" << error;
                        info->deleted.store(true, std::memory_order_release);
                    } else {
                        SLOG(WARNING) << "EAGAIN";
                    }
                    break;
                } else if (ret == 0) {
                    SLOG(WARNING) << "close connection";
                    info->deleted.store(true, std::memory_order_release);
                    break;
                } else {
                    SLOG(INFO) << "get data [size:" << ret <<"]:" << info->buffer;
                    //do something here;
                }
            }   
        }
    });

    return get_local_loop()->add_io(_queue_read_io);
}

bool TcpReaderPool::create()
{
    _queue_write_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (_queue_write_fd < 0) {
        SLOG(WARNING) << "get eventfd fail!";
        return false;
    }

    for(int i = 1; i < std::thread::hardware_concurrency(); ++i) {
        _read_pool.emplace_back(new TcpReader(_queue_write_fd, _connections));
        _read_pool.back()->run();
    }
    return true;
}

void TcpReaderPool::destroy()
{
    for (auto &reader : _read_pool) {
        reader->stop();
    }
    rpc::close(_queue_write_fd);
}

void TcpReaderPool::push(SConnection& connection)
{
    _connections.sp_push(connection);
    eventfd_write(_queue_write_fd, 1);
}

}

