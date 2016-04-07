#include "tcp_service.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "sockets.h"
#include "exception.h"
#include "loop.h"
#include "logging.h"

namespace smart {

Service::Method Service::_default_method = 
    [](const shared_json&, shared_json& response, SControl&) {
        (*response)["code"] = static_cast<int>(ErrCode::PARAM_ERROR);
        (*response)["message"] = "method not found";
    };

bool TcpService::prepare()
{
    int error = 0;
    auto fd = rpc::create_tcp(&error);
    if (fd < 0) {
        SLOG(FATAL) << "create tcp fd fail:" << error;
        return false;
    }
    
    if (!rpc::bind(fd, _port, &error)) {
        SLOG(FATAL) << "bind tcp fd fail:" << error;
        return false;
    }

    _idle_fd = open("/dev/null", O_RDONLY | O_CLOEXEC);

    _listen_io->set_fd(fd);
    _listen_io->set_events(EV_READ);
    _listen_io->on_read([this]() {
        struct sockaddr_in addr;
        int error = 0;
        auto conn_fd = rpc::accept(_listen_io->fd(), addr, &error);
        if (conn_fd < 0) {
            SLOG(WARNING) << "accept fail";
            if (error == EMFILE) {
                close(_idle_fd);
                _idle_fd = accept(_listen_io->fd(), nullptr, nullptr);
                close(_idle_fd);
                _idle_fd = open("/dev/null", O_RDONLY | O_CLOEXEC);
            }
            return;
        }

        auto ret = _conn_map.emplace(conn_fd, std::make_shared<Connection>());
        auto& info = ret.first->second;
        info->io = std::make_shared<IO>(conn_fd, EV_READ);
        info->inet_addr.addr = addr.sin_addr.s_addr;
        info->inet_addr.saddr = inet_ntoa(addr.sin_addr);
        info->inet_addr.port = addr.sin_port;

        SLOG(INFO) << "get new connection, fd[" << conn_fd 
               << "] from IP[" << info->inet_addr.saddr 
               << "] port[" << info->inet_addr.port <<"]";

        static auto parse_http_message = [this](decltype(info) &my_info) {
            if (!my_info->_deposited_msg) {
                my_info->_deposited_msg = std::make_shared<HttpMessage>();
            }   
 
            my_info->i_buffer.cut(my_info->_deposited_msg->parse(my_info->i_buffer));
            if (my_info->_deposited_msg->completed()) {
                SLOG(INFO) << "http message:" << *my_info->_deposited_msg;
                Letter letter;
                letter.first = my_info;
                my_info->_deposited_msg.swap(letter.second);

                SLOG(INFO) << "begin to parse:" << letter.second->content();
                auto response = std::make_shared<json>();
                auto ctrl = std::make_shared<Control>(letter, response);
                try {
                    auto request = std::make_shared<json>(
                            json::parse(letter.second->content()));
        
                    SLOG(INFO) << "begin to call method: " << letter.second->get_service_name()
                               << " " << letter.second->get_method_name();
                    find_service(letter.second->get_service_name())
                        ->find_method(letter.second->get_method_name())(request, response, ctrl);
                } catch (std::exception& ex) {
                    LOG(WARNING) << "catch error:" << ex.what();
                    (*response)["code"] = static_cast<int>(ErrCode::PARAM_ERROR);
                    (*response)["message"] = ex.what();
                }
            }   
        };

        info->io->on_read([&info, this]() {
            SLOG(INFO) << "read connection fd[" << info->io->fd()
                       << "] from IP[" << info->inet_addr.saddr
                       << "] port[" << info->inet_addr.port <<"]";
            int error = 0;
            ssize_t nread = info->i_buffer.append_from_fd(info->io->fd(), &error);
            if (nread == -2) {
                    parse_http_message(info);
            } else if (nread == 0) {
                SLOG(INFO) << "connection is closed, reason:" << error
                       <<". fd[" << info->io->fd()
                       << "] from IP[" << info->inet_addr.saddr
                       << "] port[" << info->inet_addr.port <<"]";
                get_local_loop()->remove_io(info->io);
                _conn_map.erase(info->io->fd());
                return;
            }
 
            parse_http_message(info);
        });
 
        get_local_loop()->add_io(info->io);
    });

    get_local_loop()->add_io(_listen_io);
    if (!rpc::listen(fd, &error)) {
        SLOG(FATAL) << "listen fd fail:" << error;
        return false;
    }

    return true;
}

void TcpService::process_end()
{
    get_local_loop()->remove_io(_listen_io);
}

}

