#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "sockets.h"
#include "exception.h"
#include "logging.h"

int rpc::create(int domain, int type, int protocol, int* error)
{
    int sockfd = ::socket(domain, type | SOCK_NONBLOCK | SOCK_CLOEXEC, protocol);
    if (sockfd < 0)
    {
        if (error) {
            *error = errno;
        }
        return -1;
    }

    return sockfd;
}

int rpc::create_tcp(int* error) {
    int sockfd = create(AF_INET, SOCK_STREAM, 0, error);
    int reuse = 1;
    ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
    ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(int));
    int nodelay = 1;
    ::setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

    return sockfd;
}

bool rpc::bind(int& sockfd, unsigned short port, int* error)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (::bind(sockfd, (struct sockaddr*) &addr, sizeof(addr)) < 0)
    {
        if (nullptr != error) {
            *error = errno;
        }
        ::close(sockfd);
        return false;
    }

    return true;
}

bool rpc::listen(int sockfd, int* error)
{
    int ret = ::listen(sockfd, SOMAXCONN);
    if (ret < 0)
    {
        if (nullptr != error) {
            *error = errno;
        }
        return false;
    }

    return true;
}

int rpc::accept(int sockfd, struct sockaddr_in& addr, int* error)
{
    socklen_t len=sizeof(addr);
    int connfd = ::accept4(sockfd, (struct sockaddr*) &addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if(connfd < 0)
    {
        if (nullptr != error) {
            *error = errno;
        }
        switch (errno)
        {
        case EBADF:
        case EFAULT:
        case EINVAL:
        case ENFILE:
        case ENOBUFS:
        case ENOMEM:
        case ENOTSOCK:
        case EOPNOTSUPP:
            {
                std::stringstream sstream;
                sstream << "socket: accept fd " << sockfd << "failed!";
                throw_system_error(sstream.str());
            }
            break;
        default:
            SLOG(ERROR) << "socket accept fd " << sockfd << " failed :" << error;
            break;
        }
    }

    int nodelay = 1;
    if(::setsockopt(connfd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay)) == -1)
    {
    }

    return connfd;
}

//to be changed
int rpc::connect(int& sockfd, const std::string& dest, int* error)
{
    /*struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family=AF_UNIX;
    strncpy(addr.sun_path, dest.c_str(),sizeof(addr.sun_path)-1);

    int connfd = connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    if(connfd < 0)
    {
        error = errno;
        Close(sockfd);
    }

    return connfd;*/
    return -1;
}

void rpc::close(int& sockfd)
{
    if(::close(sockfd) < 0) {
        std::stringstream sstream;
        sstream << "Socket: close fd " << sockfd << "failed!";
        throw_system_error(sstream.str());
    }

    sockfd = -1;
}

int rpc::nonblock(int fd, int set)
{
    int r;
    do {
        r = fcntl(fd, F_GETFL);
    } while (r == -1 && errno == EINTR);

    if (r == -1) {
        return errno;
    }

    if (!!(r & O_NONBLOCK) == !!set) {
        return 0;
    }

    if (set) {
        r |= O_NONBLOCK;
    } else {
        r &= ~O_NONBLOCK;
    }

    int flags = r;
    do {
        r = fcntl(fd, F_SETFL, flags);
    } while (r == -1 && errno == EINTR);

    if (r == -1) {
        return errno;
    }

    return 0;
}

struct sockaddr_in rpc::get_local_addr(int sockfd)
{
    struct sockaddr_in localaddr;
    memset(&localaddr, 0, sizeof(localaddr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
    if(getsockname(sockfd, (struct sockaddr *)(&localaddr), &addrlen) < 0)
    {
    }

    return localaddr;
}

