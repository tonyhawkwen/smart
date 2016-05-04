#include "customer_service.h"
#include <sstream>
#include "redis_process.h"

namespace smart {

void CustomerService::init() {
    add_method("UploadData", [](const shared_json& request, shared_json& reply, SControl& ctrl) {
        auto itr = request->find("token");
        if (itr == request->end()) {
            (*reply)["code"] = static_cast<int>(ErrCode::PARAM_ERROR); 
            (*reply)["message"] = "miss required param: token";
            return;
        }

        get_mobile_by_token(itr->get<std::string>(), 
            [request, reply, ctrl](const std::string& mobile) {
            if (mobile.empty()) {
                (*reply)["code"] = static_cast<int>(ErrCode::LOGIN_ERR);
                (*reply)["message"] = "token is wrong";
                return;
            }
                
            std::vector<std::string> redis_request;
            redis_request.emplace_back("HMSET");
            redis_request.emplace_back(mobile);
            auto itr_name = request->find("name");
            if (itr_name != request->end()) {
                redis_request.emplace_back("name");
                redis_request.emplace_back(itr_name->get<std::string>());
            }
            auto itr_income = request->find("income");
            if (itr_income != request->end()) {
                redis_request.emplace_back("income");
                redis_request.emplace_back(itr_income->get<std::string>());
            }
            auto itr_loans = request->find("has_loans");
            if (itr_loans != request->end()) {
                redis_request.emplace_back("loans");
                redis_request.emplace_back(itr_loans->get<std::string>());
            }
            if (!get_local_redis()->connected()) {
                (*reply)["code"] = static_cast<int>(ErrCode::SYSTEM_ERROR);
                (*reply)["message"] = "connect redis fail";
                return ;
            }
        
            get_local_redis()->send_request(redis_request, 
                [reply, ctrl](redisReply* redis_reply){
                if (redis_reply->type == REDIS_REPLY_STATUS) {
                    SLOG(DEBUG) << "result:" << redis_reply->str;
                    std::string result = redis_reply->str;
                    if(result == "OK") {
                        (*reply)["code"] = static_cast<int>(ErrCode::OK);
                        (*reply)["message"] = "success";
                        return;
                    }
                }
                (*reply)["code"] = static_cast<int>(ErrCode::SYSTEM_ERROR);
                (*reply)["message"] = "system error";
            });
        });
    });

    add_method("ApplyForLoan", [](const shared_json& request, shared_json& reply, SControl& ctrl) {
        auto itr = request->find("token");
        if (itr == request->end()) {
            (*reply)["code"] = static_cast<int>(ErrCode::PARAM_ERROR); 
            (*reply)["message"] = "miss required param: token";
            return;
        }

        get_mobile_by_token(itr->get<std::string>(), 
            [request, reply, ctrl](const std::string& mobile) {
            if (mobile.empty()) {
                (*reply)["code"] = static_cast<int>(ErrCode::LOGIN_ERR);
                (*reply)["message"] = "token is wrong";
                return;
            }

            if (!get_local_redis()->connected()) {
                (*reply)["code"] = static_cast<int>(ErrCode::SYSTEM_ERROR);
                (*reply)["message"] = "connect redis fail";
                return ;
            }
        
            std::string redis_request;
            SLOG(DEBUG) << "redis request:" << redis_request;
            get_local_redis()->send_request(redis_request, 
                [reply, ctrl](redisReply* redis_reply){
            });
        });        
    });
}

}

