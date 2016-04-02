#include "auth_service.h"
#include "redis_proxy.h"

namespace smart {

void AccessService::init() {
    add_method("Register", [](const shared_json& request, shared_json& reply, SControl& ctrl) {
        auto itr = request->find("mobile");
        if (itr == request->end()) {
            (*reply)["code"] = static_cast<int>(ErrCode::PARAM_ERROR);
            (*reply)["message"] = "miss required param : mobile";
            return ;
        }
        if (!itr->is_string()) {
            (*reply)["code"] = static_cast<int>(ErrCode::PARAM_ERROR);
            (*reply)["message"] = "illegal param : mobile";
            return ;
        }
        auto itr_passwd = request->find("password");
        if (itr_passwd == request->end()) {
            (*reply)["code"] = static_cast<int>(ErrCode::PARAM_ERROR);
            (*reply)["message"] = "miss required param : password";
            return ;
        }
        if (!itr_passwd->is_string()) {
            (*reply)["code"] = static_cast<int>(ErrCode::PARAM_ERROR);
            (*reply)["message"] = "illegal param : password";
            return ;
        }

        if (!get_local_redis()->connected()) {
            (*reply)["code"] = static_cast<int>(ErrCode::SYSTEM_ERROR);
            (*reply)["message"] = "system error";
            return ;
        }
        //FIX ME: check if user is phone number or email
        std::string redis_request("HSETNX ");
        redis_request.append(itr->get<std::string>());
        redis_request.append(" password ");
        redis_request.append(itr_passwd->get<std::string>());
        get_local_redis()->send_request(redis_request, 
            [reply, ctrl](redisReply* redis_reply){
                if (redis_reply->type == REDIS_REPLY_INTEGER
                    && redis_reply->integer == 1) {
                    (*reply)["code"] = static_cast<int>(ErrCode::OK);
                    (*reply)["message"] = "success";
                    return;
                }
                (*reply)["code"] = static_cast<int>(ErrCode::REGISTERED);
                (*reply)["message"] = "this mobile has been registerd.";
            }
        );
    });

    add_method("Login", [](const shared_json& request, shared_json& reply, SControl& ctrl) {
        auto itr = request->find("mobile");
        if (itr == request->end()) {
            (*reply)["code"] = static_cast<int>(ErrCode::PARAM_ERROR);
            (*reply)["message"] = "miss required param : mobile";
            return ;
        }
        if (!itr->is_string()) {
            (*reply)["code"] = static_cast<int>(ErrCode::PARAM_ERROR);
            (*reply)["message"] = "illegal param : mobile";
            return ;
        }
        auto itr_passwd = request->find("password");
        if (itr_passwd == request->end()) {
            (*reply)["code"] = static_cast<int>(ErrCode::PARAM_ERROR);
            (*reply)["message"] = "miss required param : password";
            return ;
        }
        if (!itr_passwd->is_string()) {
            (*reply)["code"] = static_cast<int>(ErrCode::PARAM_ERROR);
            (*reply)["message"] = "illegal param : password";
            return ;
        }

        if (!get_local_redis()->connected()) {
            (*reply)["code"] = static_cast<int>(ErrCode::SYSTEM_ERROR);
            (*reply)["message"] = "system error";
            return ;
        }

        std::string redis_request("HGET ");
        redis_request.append(itr->get<std::string>());
        redis_request.append(" password");
        std::string password = itr_passwd->get<std::string>();
        get_local_redis()->send_request(redis_request, 
            [password, reply, ctrl](redisReply* redis_reply){
                if (redis_reply->type == REDIS_REPLY_STRING) {
                    if (password == redis_reply->str) {
                        (*reply)["code"] = static_cast<int>(ErrCode::OK);
                        (*reply)["message"] = "success";
                        return;
                    }
                }
                (*reply)["code"] = static_cast<int>(ErrCode::LOGIN_ERR);
                (*reply)["message"] = "mobile number/password is wrong";
            }
        );
    });

}

}

