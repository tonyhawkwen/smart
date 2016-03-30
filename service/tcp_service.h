#ifndef SERVICE_TCP_SERVICE_H
#define SERVICE_TCP_SERVICE_H

#include <map>
#include "loop_thread.h"
#include "tcp_reader.h"

namespace smart {

class TcpService : public LoopThread {
public:
    using EventCallback = std::function<void()>;

    TcpService():
        LoopThread("tcp_service"),
        _port(1080),
        _listen_io(new IO) {}
    
    TcpService(unsigned short port):
        LoopThread("tcp_service"),
        _port(port),
        _listen_io(new IO) {}

    virtual ~TcpService() {}

protected:
    bool prepare() override;
    void process_end() override;

private:
    unsigned short _port;
    std::shared_ptr<IO> _listen_io;
    int _idle_fd;
    TcpReaderPool _read_pool;
};

}

#endif /*SERVICE_TCP_SERVICE_H*/
