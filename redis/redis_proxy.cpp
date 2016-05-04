#include "redis_proxy.h"
#include "loop.h"
#include "logging.h"

namespace smart {

std::shared_ptr<RedisProxy> get_local_redis()
{
    static thread_local std::shared_ptr<RedisProxy> s_local_redis;
    if (!s_local_redis) {
        s_local_redis.reset(new RedisProxy);
    }
    if (!s_local_redis->connected()) {
        s_local_redis->connect();
    }
    return s_local_redis;
}

RedisProxy::RedisProxy()
    : _context(nullptr),
      _connected(false),
      _reading(false),
      _writting(false)
{
}

RedisProxy::~RedisProxy()
{
 /*   if (_connected && nullptr != _context) {
        redisAsyncDisconnect(_context);
    }*/

    if (nullptr != _context) {
        redisAsyncFree(_context);
        _context = nullptr;
    }
}

std::string RedisProxy::_ip = "127.0.0.1";
int RedisProxy::_port = 6379;

bool RedisProxy::init(const std::string& ip, int port)
{
    _ip = ip;
    _port = port;

    return true;
}

void RedisProxy::connect()
{
    if(_connected) {
        return;
    }
    SLOG(DEBUG) << "redis connect begin...";

    _context = redisAsyncConnect(_ip.c_str(), _port);
    if(!_context)
    {
        SLOG(WARNING) << "allocate redis async connection fail!";
        return;
    }

    if(_context->err != 0)
    {
        SLOG(WARNING) << "connect fail[" << _context->err << "]";
        redisAsyncFree(_context);
        _context = nullptr;
        return;
    }

    _context->data = this;
    _context->ev.data = this;
    _context->ev.addRead = RedisProxy::redis_add_read;
    _context->ev.delRead = RedisProxy::redis_del_read;
    _context->ev.addWrite = RedisProxy::redis_add_write;
    _context->ev.delWrite = RedisProxy::redis_del_write;
    _context->ev.cleanup = RedisProxy::redis_cleanup;

    
    _read_io = std::make_shared<IO>(_context->c.fd, EV_READ, false);//FIXME:add EV_ET in future
    _read_io->on_read([this]() {
        LOG(INFO) << "on read";
        redisAsyncHandleRead(_context);        
    });
    _write_io = std::make_shared<IO>(_context->c.fd, EV_WRITE, false);//FIXME:add EV_ET in future
    LOG(INFO) << "redis fd:" << _context->c.fd;
    _write_io->on_write([this]() {
        LOG(INFO) << "on write";
        redisAsyncHandleWrite(_context);
    });

    _connected = get_local_loop()->add_io(_read_io, false)
        && get_local_loop()->add_io(_write_io, false);

    redisAsyncSetConnectCallback(_context, RedisProxy::handle_connect);
    redisAsyncSetDisconnectCallback(_context, RedisProxy::handle_disconnect);
}

void RedisProxy::redis_add_read(void *privdata)
{
    RedisProxy* pThis = static_cast<RedisProxy*>(privdata);
    if (!pThis->_reading) {
        pThis->_reading = true;
        get_local_loop()->start_io(pThis->_read_io);
        LOG(INFO) << "redis add read";
    }
}

void RedisProxy::redis_del_read(void *privdata)
{
    RedisProxy* pThis = static_cast<RedisProxy*>(privdata);
    if (pThis->_reading) {
        pThis->_reading = false;
        get_local_loop()->stop_io(pThis->_read_io);
        LOG(INFO) << "redis del read";
    }
}

void RedisProxy::redis_add_write(void *privdata)
{
    RedisProxy* pThis = static_cast<RedisProxy*>(privdata);
    if (!pThis->_writting) {
        pThis->_writting = true;
        get_local_loop()->start_io(pThis->_write_io);
        LOG(INFO) << "redis add write";
    }
}

void RedisProxy::redis_del_write(void *privdata)
{
    RedisProxy* pThis = static_cast<RedisProxy*>(privdata);
    if (pThis->_writting) {
        pThis->_writting = false;
        get_local_loop()->stop_io(pThis->_write_io);
        LOG(INFO) << "redis del write";
    }
}

void RedisProxy::redis_cleanup(void *privdata)
{
    redis_del_read(privdata);
    redis_del_write(privdata);
}

void RedisProxy::handle_connect(const redisAsyncContext *c, int status)
{
    if (status != REDIS_OK) {
        LOG(WARNING) << "redis connect rror:" << c->errstr;
        return;
    }
    LOG(INFO) << "redis connected";
}

void RedisProxy::handle_disconnect(const redisAsyncContext *c, int status)
{
    if (status != REDIS_OK) {
        LOG(WARNING) << "redis disconnect rror:" << c->errstr;
        return;
    }
    LOG(INFO) << "redis disconnected";
}

void RedisProxy::redis_callback(struct redisAsyncContext*, void* r, void* privdata)
{
    redisReply* reply = static_cast<redisReply*>(r);
    RedisProxy* pThis = static_cast<RedisProxy*>(privdata);
    auto fn = pThis->_callbacks.front();
    pThis->_callbacks.pop();
    if (reply->type == REDIS_REPLY_ERROR) {
        SLOG(DEBUG) << "redis error:" << reply->str;
    }
    fn(reply);
}

}

