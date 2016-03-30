#include "tcp_reader.h"
#include <sys/uio.h>
#include <sys/eventfd.h>
#include "sockets.h"
#include "loop.h"
#include "logging.h"
#include "service_pool.h"

namespace smart {

TcpReader::TcpReader(int fd, MPMCQueue<SConnection>& connections):
    LoopThread("tcp_reader"),
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
            SConnection one_conn;
            if (!_connections->pop(&one_conn)) {
                break;
            }
            
            if (!one_conn) {
                SLOG(WARNING) << "info empty!";
                continue;
            }
         
            auto ret = _conn_map.emplace(one_conn->io->fd(), std::move(one_conn)); 
            auto& info = ret.first->second;
            static auto parse_http_message = [](decltype(info) &my_info) {
                if (!my_info->_deposited_msg) {
                    my_info->_deposited_msg = std::make_shared<HttpMessage>();
                }

                SLOG(INFO) << "get data:" << my_info->i_buffer;
                my_info->i_buffer.cut(my_info->_deposited_msg->parse(my_info->i_buffer));
                if (my_info->_deposited_msg->completed()) {
                    SLOG(INFO) << "http message:" << my_info->_deposited_msg;
                    Letter letter;
                    letter.first = my_info;
                    my_info->_deposited_msg.swap(letter.second);
                    ServicePool::get_instance().send(std::move(letter));
                }   
            };

            info->io->on_read([&info, this]() {
                SLOG(INFO) << "read connection fd[" << info->io->fd()
                           << "] from IP[" << info->inet_addr.saddr
                           << "] port[" << info->inet_addr.port <<"]";
                int error = 0;
                ssize_t nread = 0;
                while (1) {
                    nread = info->i_buffer.append_from_fd(info->io->fd(), &error);
                    if (nread == -2) {
                        parse_http_message(info);
                    } else if (nread <= 0) {
                        break;
                    }
                }

                if (nread == 0 || (nread < 0 && error != EAGAIN)) {
                    SLOG(WARNING) << "close connection, reason:" << error;
                    get_local_loop()->remove_io(info->io);
                    _conn_map.erase(info->io->fd());
                    SLOG(INFO) << "connection is closed fd[" << info->io->fd()
                           << "] from IP[" << info->inet_addr.saddr
                           << "] port[" << info->inet_addr.port <<"]";
                    return;
                }

                parse_http_message(info);
            });

            get_local_loop()->add_io(info->io);
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
        if(!_read_pool.back()->run()) {
            return false;
        }
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

void TcpReaderPool::push(SConnection&& connection)
{
    _connections.sp_push(connection);
    eventfd_write(_queue_write_fd, 1);
}

}

