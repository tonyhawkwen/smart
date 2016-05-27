#ifndef REDIS_REDIS_PROXY_H
#define REDIS_REDIS_PROXY_H

#include <hiredis/hiredis.h> 
#include <hiredis/async.h>
#include <memory>
#include <functional>
#include <queue>
#include <vector>
#include "io.h"

namespace smart {

class RedisProxy {
public:
    using RedisCallback = std::function<void(redisReply*)>;
    RedisProxy();
    ~RedisProxy();

    static bool init(const std::string& ip, int port);
    void connect();
    bool connected() { return _connected; }

    template<typename T>
    bool send_request(const std::vector<std::string>& commands, T&& fn)
    {
        if (nullptr == _context || commands.empty()) {
            return false;
        }
        _callbacks.emplace(std::forward<T>(fn));
        auto cmd_data = new const char*[commands.size()];
        auto cmd_sizes = new size_t[commands.size()];
        for (auto i = 0; i <commands.size(); ++i) {
            cmd_data[i] = commands[i].c_str();
            cmd_sizes[i] = commands[i].length();
        }

        redisAsyncCommandArgv(_context,
                              redis_callback,
                              this,
                              commands.size(),
                              cmd_data,
                              cmd_sizes);
        delete [] cmd_data;
        delete [] cmd_sizes;
        return true;
    }

    template<typename T>
    bool send_request(const std::string& command, T&& fn)
    {
        if (nullptr == _context || command.empty()) {
            return false;
        }
        _callbacks.emplace(std::forward<T>(fn));
        redisAsyncCommand(_context, redis_callback, this, command.c_str());
        return true;
    }

    bool send_request(const std::string& command)
    {
        if (nullptr == _context || command.empty()) {
            return false;
        }
        _callbacks.emplace([](redisReply*) {});
        redisAsyncCommand(_context, redis_callback, this, command.c_str());
        return true;
    }

private:
    static void redis_callback(struct redisAsyncContext*, void*, void*);

    static void redis_add_read(void *privdata);
    static void redis_del_read(void *privdata);
    static void redis_add_write(void *privdata);
    static void redis_del_write(void *privdata);
    static void redis_cleanup(void *privdata);
    static void handle_connect(const redisAsyncContext *c, int status);
    static void handle_disconnect(const redisAsyncContext *c, int status);

    static std::string _ip;
    static int _port;
    redisAsyncContext* _context;
    std::shared_ptr<IO> _read_io;
    std::shared_ptr<IO> _write_io;
    bool _connected;
    bool _reading;
    bool _writting;
    std::queue<RedisCallback> _callbacks;
};

std::shared_ptr<RedisProxy> get_local_redis();

}

#endif
