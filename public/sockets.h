#ifndef PUBLIC_SOCKETS_H
#define PUBLIC_SOCKETS_H
#include <string>

namespace rpc {

int create_tcp(int* error);
int create(int domain, int type, int protocol, int* error);
bool bind(int& sockfd, unsigned short port, int& error);
bool listen(int sockfd, int& error);
int accept(int sockfd, struct sockaddr_in& addr, int& error);
int connect(int& sockfd, const std::string& dest, int& error);
void close(int& sockfd);
int nonblock(int fd, int set);
struct sockaddr_in get_local_addr(int sockfd);

}


#endif

