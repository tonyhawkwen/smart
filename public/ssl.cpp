#include "ssl.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <openssl/err.h>
#include "logging.h"

namespace rpc {

const char* certificate = "/home/ubuntu/workspace/private/openssl/ispectre.crt";
const char* private_key = "/home/ubuntu/workspace/private/openssl/ispectre.key.unsecure";

std::ostream& operator<<(std::ostream& os, const SSLError& ssl) {
    char buf[128];
    ERR_error_string_n(ssl.error, buf, sizeof(buf));
    return os << buf;
}

void init_ssl()
{
    SSL_library_init();
    SSL_load_error_strings();
}

std::shared_ptr<SSL_CTX> create_ssl_context() 
{
    auto ptr = SSL_CTX_new(SSLv23_server_method());
    do {
        if (ptr == nullptr) {
            SLOG(WARNING) << "ssl ctx allocate fail.";
            break;
        }

        SSL_CTX_set_verify(ptr, SSL_VERIFY_NONE, NULL);
        if (SSL_CTX_use_certificate_file(ptr, certificate, SSL_FILETYPE_PEM) != 1) {
            SLOG(WARNING) << "Fail to load certificate : " << SSLError(ERR_get_error());
            break;
        }

        if (SSL_CTX_use_PrivateKey_file(ptr, private_key, SSL_FILETYPE_PEM) != 1) {
            SLOG(WARNING) << "Fail to load private key:" << SSLError(ERR_get_error());
            break;
        }

        if (SSL_CTX_check_private_key(ptr) != 1) {
            SLOG(WARNING) << "Fail to verify private key:" << SSLError(ERR_get_error());
            break;
        }

        return std::shared_ptr<SSL_CTX>(ptr, [](SSL_CTX* p){ SSL_CTX_free(p); });
    } while (0);
    
    if (nullptr != ptr) {
        SSL_CTX_free(ptr);
    }

    return std::shared_ptr<SSL_CTX>();
}

std::shared_ptr<SSL> create_ssl_session(
        std::shared_ptr<SSL_CTX>& ctx, int fd, bool server_mode)
{
    SSL* ssl = SSL_new(ctx.get());
    if (nullptr == ssl) {
        SLOG(WARNING) << "Fail to allocate ssl:" << SSLError(ERR_get_error());
        return std::shared_ptr<SSL>();
    }

    if (SSL_set_fd(ssl, fd) != 1) {
        SLOG(WARNING) << "Fail to bind ssl to fd:" << SSLError(ERR_get_error());
        SSL_free(ssl);
        return std::shared_ptr<SSL>();
    }

    if (server_mode) {
        SSL_set_accept_state(ssl);
    } else {
        SSL_set_connect_state(ssl);
    }

    return std::shared_ptr<SSL>(ssl, [](SSL* p){ SSL_free(p); });
}

bool ssl_shakehand(std::shared_ptr<SSL>& ssl, int* error)
{
    auto ret = SSL_do_handshake(ssl.get());
    if (ret != 1) {
        *error = SSL_get_error(ssl.get(), ret);
        SLOG(WARNING) << "Fail to do handshake:" << *error;
        SLOG(WARNING) << "Fail to do handshake:" << SSLError(ERR_get_error());
        return false;
    } else {
        SLOG(INFO) << "handshake success!";
        return true;
    }
}

SslCheck is_ssl(int fd)
{
    char header[6];
    auto nr = recv(fd, header, sizeof(header), MSG_PEEK);
    if (nr < 6) {
        return SslCheck::NOT_SSL;
    }

    if ((header[0] == 0x16 && header[5] == 0x01)
        || ((header[0] & 0x80) == 0x80 && header[2] == 0x01)) {
        return SslCheck::IS_SSL;
    }
    return SslCheck::NOT_SSL;
}

}
