#ifndef SERVICE_TCP_READER_H
#define SERVICE_TCP_READER_H

#include <vector>
#include <memory>
#include "io.h"
#include "service.h"
#include "mpmc_queue.hpp"
#include "inet_address.h"

namespace smart {

class TcpReader : public Service {
public:
    TcpReader(int fd, MPMCQueue<SConnection>& infos);
    virtual ~TcpReader();
protected:
    bool prepare() override;

private:
    std::shared_ptr<IO> _queue_read_io;
    MPMCQueue<SConnection>* _connections;
};

class TcpReaderPool {
public:
    TcpReaderPool(): _queue_write_fd(-1) {}
    ~TcpReaderPool() {}

    bool create();
    void destroy();
    void push(SConnection&);

private:
    int _queue_write_fd;
    std::vector<std::unique_ptr<TcpReader>> _read_pool;
    MPMCQueue<SConnection> _connections;
};

}
#endif /*SERVICE_TCP_READER_H*/
