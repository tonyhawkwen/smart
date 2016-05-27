#ifndef REDIS_REDIS_PROCESS_H
#define REDIS_REDIS_PROCESS_H

#include "redis_proxy.h"

namespace smart {

std::string generate_token(const std::string mobile, const std::string password);

void refresh_token(const std::string mobile);

template<typename T>
void get_mobile_by_token(const std::string& token, T&& fn)
{
    std::string redis_request("GET TOKEN_");
    redis_request.append(token);
    get_local_redis()->send_request(redis_request,
        [func = std::forward<T>(fn)](redisReply* redis_reply) {
            std::string mobile;
            if (redis_reply->type == REDIS_REPLY_STRING) {
                mobile = redis_reply->str;
            }
            func(mobile);
        });
}

}

#endif /*REDIS_REDIS_PROCESS_H*/
