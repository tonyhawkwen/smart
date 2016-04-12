#ifndef PUBLIC_SSL_H
#define PUBLIC_SSL_H

#include <memory>
#include <openssl/ssl.h>

namespace rpc {

enum class SslCheck {
    UNKNOWN = 0,
    IS_SSL = 1,
    NOT_SSL = 2
};

struct SSLError {
    explicit SSLError(unsigned long e): error(e) { }
    unsigned long error;
};

std::ostream& operator<<(std::ostream& os, const SSLError& ssl);

void init_ssl();
std::shared_ptr<SSL_CTX> create_ssl_context();
std::shared_ptr<SSL> create_ssl_session(std::shared_ptr<SSL_CTX>& ctx, int fd, bool server_mode);
bool ssl_shakehand(std::shared_ptr<SSL>& ssl, int* error);
SslCheck is_ssl(int fd);

}

#endif /*PUBLIC_SSH_H*/
