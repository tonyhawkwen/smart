#ifndef SERVICE_TCP_SERVICE_H
#define SERVICE_TCP_SERVICE_H

#include <list>
#include "service.h"
#include "io.h"
#include "inet_address.h"

namespace smart {

struct Connection {
    std::shared_ptr<IO> io;
    InetAddress inet_addr;
};

class TcpService : public Service {
public:
    using EventCallback = std::function<void()>;

    TcpService():
        Service("tcp_service"),
        _port(1080),
        _listen_io(new IO) {}
    
    TcpService(unsigned short port):
        Service("tcp_service"),
        _port(port),
        _listen_io(new IO) {}

    virtual ~TcpService() {}

protected:
    bool prepare() override;

private:
    unsigned short _port;
    std::shared_ptr<IO> _listen_io;
    int _idle_fd;
    std::list<Connection> _connections;
};

}

#endif /*SERVICE_TCP_SERVICE_H*/
