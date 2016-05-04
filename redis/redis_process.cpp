#include "redis_process.h"
#include <sstream>
#include <chrono>
#include "md5.h"

namespace smart {

static constexpr auto private_key = "6b7c9c763fad3e@#99e44!$b6ffb*51382bb6";

std::string generate_token(const std::string mobile, const std::string password)
{
    //generate md5 token
    std::stringstream ss;
    ss << mobile << private_key << password
       << std::chrono::system_clock::now().time_since_epoch().count();
    auto md5 = MD5(ss.str()).toStr();
    std::string redis_request("SETEX TOKEN_");
    redis_request.append(md5);
    redis_request.append(" 3600 ");
    redis_request.append(mobile);
    get_local_redis()->send_request(redis_request);

    return md5;
}

void refresh_token(const std::string mobile)
{
    std::string redis_request("EXPIRE TOKEN_");
    redis_request.append(mobile);
    redis_request.append(" 3600");
    get_local_redis()->send_request(redis_request);
}

}
