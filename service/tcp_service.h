#ifndef SERVICE_TCP_SERVICE_H
#define SERVICE_TCP_SERVICE_H

#include <list>
#include "service.h"
#include "tcp_reader.h"

namespace smart {

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

    void stop() override;

protected:
    bool prepare() override;

private:
    unsigned short _port;
    std::shared_ptr<IO> _listen_io;
    int _idle_fd;
    std::list<SConnection> _connections;
    TcpReaderPool _read_pool;
};

}

#endif /*SERVICE_TCP_SERVICE_H*/
