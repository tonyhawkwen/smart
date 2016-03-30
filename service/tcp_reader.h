#ifndef SERVICE_TCP_READER_H
#define SERVICE_TCP_READER_H

#include <map>
#include <vector>
#include <memory>
#include "io.h"
#include "loop_thread.h"
#include "mpmc_queue.hpp"
#include "service_data.h"

namespace smart {

class TcpReader : public LoopThread {
public:
    TcpReader(int fd, MPMCQueue<SConnection>& infos);
    virtual ~TcpReader();
protected:
    bool prepare() override;

private:
    std::shared_ptr<IO> _queue_read_io;
    MPMCQueue<SConnection>* _connections;
    std::map<int, SConnection> _conn_map;
};

class TcpReaderPool {
public:
    TcpReaderPool(): _queue_write_fd(-1) {}
    ~TcpReaderPool() {}

    bool create();
    void destroy();
    void push(SConnection&&);

private:
    int _queue_write_fd;
    std::vector<std::unique_ptr<TcpReader>> _read_pool;
    MPMCQueue<SConnection> _connections;
};

}
#endif /*SERVICE_TCP_READER_H*/
