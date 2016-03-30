#include "tcp_service.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "sockets.h"
#include "exception.h"
#include "loop.h"
#include "logging.h"

namespace smart {

bool TcpService::prepare()
{
    if (!_read_pool.create()) {
        SLOG(FATAL) << "create read pool fail";
        return false;
    }

    int error = 0;
    auto fd = rpc::create_tcp(&error);
    if (fd < 0) {
        SLOG(FATAL) << "create tcp fd fail:" << error;
        return false;
    }
    
    if (!rpc::bind(fd, _port, &error)) {
        SLOG(FATAL) << "bind tcp fd fail:" << error;
        return false;
    }

    _idle_fd = open("/dev/null", O_RDONLY | O_CLOEXEC);

    _listen_io->set_fd(fd);
    _listen_io->set_events(EV_READ | EV_ET);
    _listen_io->on_read([this]() {
        while (1) {
            struct sockaddr_in addr;
            int error = 0;
            auto conn_fd = rpc::accept(_listen_io->fd(), addr, &error);
            if (conn_fd < 0) {
                if (error == EAGAIN) {
                    break;
                }
                SLOG(WARNING) << "accept fail";
                /*if (error == EMFILE) {
                    close(_idle_fd);
                    _idle_fd = accept(_listen_io->fd(), nullptr, nullptr);
                    close(_idle_fd);
                    _idle_fd = open("/dev/null", O_RDONLY | O_CLOEXEC);
                }*/
                continue;
            }

            auto info = std::make_shared<Connection>();
            info->io = std::make_shared<IO>(conn_fd, EV_READ | EV_ET);
            info->inet_addr.addr = addr.sin_addr.s_addr;
            info->inet_addr.saddr = inet_ntoa(addr.sin_addr);
            info->inet_addr.port = addr.sin_port;
            _read_pool.push(std::move(info));

            SLOG(INFO) << "get new connection, fd[" << conn_fd 
                   << "] from IP[" << info->inet_addr.saddr 
                   << "] port[" << info->inet_addr.port <<"]";
        }
    });

    get_local_loop()->add_io(_listen_io);
    if (!rpc::listen(fd, &error)) {
        SLOG(FATAL) << "listen fd fail:" << error;
        return false;
    }

    return true;
}

void TcpService::process_end()
{
    _read_pool.destroy();
}

}

