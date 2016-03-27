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
    _listen_io->set_events(EV_READ);
    _listen_io->on_read([this]() {
        struct sockaddr_in addr;
        int error = 0;
        auto conn_fd = rpc::accept(_listen_io->fd(), addr, &error);
        if (conn_fd < 0) {
            SLOG(WARNING) << "accept fail";
            if (error == EMFILE) {
                close(_idle_fd);
                _idle_fd = accept(_listen_io->fd(), nullptr, nullptr);
                close(_idle_fd);
                _idle_fd = open("/dev/null", O_RDONLY | O_CLOEXEC);
            }

            return;
        }
        
        _connections.emplace_back();
        auto& info = _connections.back();
        info.io.reset(new IO(conn_fd, EV_READ | EV_ET));
        info.inet_addr.addr = addr.sin_addr.s_addr;
        info.inet_addr.saddr = inet_ntoa(addr.sin_addr);
        info.inet_addr.port = addr.sin_port;
        info.io->on_read([&info](){
            SLOG(INFO) << "on read";
        });

        get_local_loop()->add_io(info.io);
        SLOG(INFO) << "get new connection, fd[" << conn_fd 
                   << "] from IP[" << info.inet_addr.saddr 
                   << "] port[" << info.inet_addr.port <<"]";
    });

    get_local_loop()->add_io(_listen_io);
    if (!rpc::listen(fd, &error)) {
        SLOG(FATAL) << "listen fd fail:" << error;
        return false;
    }

    return true;
}

}

