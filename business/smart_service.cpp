#include "smart_service.h"
#include <sstream>
#include "redis_process.h"

namespace smart {

void SmartService::parse(SConnection& info) {

    SLOG(WARNING) << "message from smart gateway";
    SmartProtocal pro;
    auto nread = info->i_buffer.append_from_fd(info->io->fd(), nullptr);
    //LOG(INFO) << "recieve buffer:\n" << info->i_buffer;
    memmove((void*)&pro, 
            (const void*)info->i_buffer.str().first,
             sizeof(SmartProtocal));
    size_t data_length = (size_t)pro.length - sizeof(SmartProtocal);
    data_length -= 2;
    unsigned char* data_str = new unsigned char[data_length];
    info->i_buffer.cut(sizeof(SmartProtocal));
    memmove((void*)data_str, (const void*)info->i_buffer.str().first, data_length);
    info->i_buffer.clear();
    //default 3
    std::vector<TLV> _data;
    for (auto i = 0; i < data_length; i += 3) {
        _data.emplace_back((size_t)data_str[i],
                           (size_t)data_str[i + 1],
                           (size_t)data_str[i + 2]);
    }

    delete [] data_str;

    //默认用户为18502560763，存储当前开关状态
    std::string lua_script = "local ret = redis.call('HGET', KEYS[1], 'request_status')\n"
                             "redis.call('HSET', KEYS[1], 'status', KEYS[2])\n"
                             "if ret and ret ~= KEYS[2] then\n"
                             "return ret\n"
                             "else\n"
                             "return ''\n"
                             "end";
    for (auto& v : _data) {
        SLOG(WARNING) << v;
        std::vector<std::string> redis_request;
        redis_request.emplace_back("EVAL");
        redis_request.emplace_back(lua_script);
        redis_request.emplace_back("2");
        char switch_name[40];
        char switch_val[16];
        snprintf(switch_name, sizeof(switch_name), "18502560763_switch_%lu", v.type);
        snprintf(switch_val, sizeof(switch_val), "%lu", v.value);
        redis_request.emplace_back(switch_name);
        redis_request.emplace_back(switch_val);
        if (get_local_redis()->connected()) {
            get_local_redis()->send_request(redis_request, 
                [info, pro, v](redisReply* redis_reply) mutable {
                if (redis_reply->type == REDIS_REPLY_STRING) {
                    SLOG(DEBUG) << "result:" << redis_reply->str;
                    if (0 < strlen(redis_reply->str)) {
                        pro.length = 0x1C;//TO FIX
                        info->o_buffer.append((const unsigned char*)&pro, sizeof(pro));
                        info->o_buffer.append(std::string(1, (char)v.type));
                        info->o_buffer.append(std::string(1, (char)0x1));
                        info->o_buffer.append(std::string(1, redis_reply->str[0] == '0' ? (char)0x0 : (char)0x1));
                        info->o_buffer.append(std::string(1, (char)0x16));
                        info->o_buffer.cut_into_fd(info->io->fd(), 0);
                    }
                }
            });
        }
    }


    
}

}

