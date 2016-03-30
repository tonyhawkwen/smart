#ifndef SERVICE_SERVICE_DATA_H
#define SERVICE_SERVICE_DATA_H

#include <string>
#include <unistd.h>
#include <memory>
#include <atomic>
#include "io.h"
#include "buffer.h"
#include "http_message.h"

namespace smart {

struct InetAddress {
    uint32_t addr;
    std::string saddr;
    uint16_t port;

    InetAddress() : addr(0), port(0){}
    InetAddress(const InetAddress& right):
        addr(right.addr),
        saddr(right.saddr),
        port(right.port){
        }

    InetAddress(InetAddress&& right):
        addr(right.addr),
        saddr(std::move(right.saddr)),
        port(right.port){
            right.addr = 0;
            right.port = 0;
        }

    InetAddress& operator= (const InetAddress& right)
    {
        if(&right == this)
        {
            return *this;
        }

        addr = right.addr;
        saddr = right.saddr;
        port = right.port;

        return *this;
    }

    InetAddress& operator= (InetAddress&& right)
    {
        if(&right == this)
        {
            return *this;
        }

        addr = right.addr;
        saddr = std::move(right.saddr);
        port = right.port;
        right.addr = 0;
        right.port = 0;

        return *this;
    }
};

struct Connection {
    std::shared_ptr<IO> io;
    InetAddress inet_addr;
    Buffer i_buffer;
    Buffer o_buffer;
    SHttpMessage _deposited_msg;
};

using SConnection = std::shared_ptr<Connection>;
using Letter = std::pair<SConnection, SHttpMessage>;
}

#endif /*SERVICE_SERVICE_DATA_H*/
