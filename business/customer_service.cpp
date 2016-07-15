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
            auto itr_company = request->find("company");
            if (itr_company != request->end()) {
                redis_request.emplace_back("company");
                redis_request.emplace_back(itr_company->get<std::string>());
            }
            auto itr_income = request->find("income");
            if (itr_income != request->end()) {
                redis_request.emplace_back("income");
                redis_request.emplace_back(itr_income->get<std::string>());
            }
            auto itr_loans = request->find("has_loans");
            if (itr_loans != request->end()) {
                redis_request.emplace_back("has_loans");
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

    add_method("GetBaseInfo", [](const shared_json& request, shared_json& reply, SControl& ctrl) {
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
            redis_request.emplace_back("HGETALL");
            redis_request.emplace_back(mobile);

            if (!get_local_redis()->connected()) {
                (*reply)["code"] = static_cast<int>(ErrCode::SYSTEM_ERROR);
                (*reply)["message"] = "connect redis fail";
                return ;
            }

            get_local_redis()->send_request(redis_request, 
                [reply, ctrl](redisReply* redis_reply){
                if (redis_reply->type == REDIS_REPLY_ARRAY) {
                    std::string key;
                    for (auto i = 0; i < redis_reply->elements; ++i) {
                        if (key.empty()) {
                            key = redis_reply->element[i]->str;
                            continue;
                        } else if (key == "password") {
                            key = "";
                            continue;
                        }
                        SLOG(WARNING) << " key:" << key;
                        (*reply)[key] = redis_reply->element[i]->str;
                        key = "";
                    }
                    (*reply)["code"] = static_cast<int>(ErrCode::OK);
                    (*reply)["message"] = "success";
                    return;
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

    add_method("GetLocation", [](const shared_json& request, shared_json& reply, SControl& ctrl) {
        auto itr = request->find("longitude");
        if (itr != request->end()) {
            SLOG(DEBUG) << "longitude:" << itr->get<std::string>();
        }
        itr = request->find("latitude");
        if (itr != request->end()) {
            SLOG(DEBUG) << "latitude:" << itr->get<std::string>();
        }
        itr = request->find("altitude");
        if (itr != request->end()) {
            SLOG(DEBUG) << "altitude:" << itr->get<std::string>();
        }
        (*reply)["code"] = static_cast<int>(ErrCode::OK);
        (*reply)["message"] = "success";
    });

    //for smart
    add_method("SetSwitchInfo", [](const shared_json& request, shared_json& reply, SControl& ctrl) {
        SLOG(WARNING) << "*******SetSwitchInfo********"; 
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

            auto itr = request->find("switch");
            if (itr != request->end()) {
                auto infos = itr->get<std::vector<json>>();
                for (auto& info : infos) {
                    auto itr_name = info.find("name");
                    if (itr_name == info.end()) {
                        continue;
                    }
                    auto itr_status = info.find("status");
                    if (itr_status == info.end()) {
                        continue;
                    }
                    
                    std::vector<std::string> redis_request;
                    redis_request.emplace_back("HSET");
                    redis_request.emplace_back(itr_name->get<std::string>());
                    redis_request.emplace_back("request_status");
                    redis_request.emplace_back(itr_status->get<std::string>());
                    
                    if (!get_local_redis()->connected()) {
                        (*reply)["code"] = static_cast<int>(ErrCode::SYSTEM_ERROR);
                        (*reply)["message"] = "connect redis fail";
                        return;
                    }
                    get_local_redis()->send_request(redis_request, 
                        [reply](redisReply* redis_reply){
                    });
                }
            }
            (*reply)["code"] = static_cast<int>(ErrCode::OK);
            (*reply)["message"] = "success";
        });
    });

    add_method("GetSwitchInfo", [](const shared_json& request, shared_json& reply, SControl& ctrl) {
        SLOG(WARNING) << "*******GetSwitchInfo********";
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
            redis_request.emplace_back("KEYS");
            redis_request.emplace_back(mobile + "_switch_*");
            if (!get_local_redis()->connected()) {
                (*reply)["code"] = static_cast<int>(ErrCode::SYSTEM_ERROR);
                (*reply)["message"] = "connect redis fail";
                return ;
            }

            SLOG(WARNING) << "*******GetSwitchInfo 2********";
            get_local_redis()->send_request(redis_request, 
                [reply, ctrl](redisReply* redis_reply){
                if (redis_reply->type == REDIS_REPLY_ARRAY) {
                    if (redis_reply->elements == 0) {
                        (*reply)["code"] = static_cast<int>(ErrCode::OK);
                        (*reply)["message"] = "success";
                        return;
                    }
                    for (auto i = 0; i < redis_reply->elements; ++i) {
                        std::string key = redis_reply->element[i]->str;
                        SLOG(WARNING) << " key:" << key;
                        std::vector<std::string> redis_req;
                        redis_req.emplace_back("HGET");
                        redis_req.emplace_back(key);
                        redis_req.emplace_back("status");
                        
                        get_local_redis()->send_request(redis_req,
                            [key, reply, ctrl](redisReply* redis_rep) {
                            if (redis_rep->type == REDIS_REPLY_STRING) {
                                json one_switch;
                                one_switch["name"] = key;
                                one_switch["status"] = redis_rep->str;
                                (*reply)["switchs"].push_back(one_switch);                          
                                (*reply)["code"] = static_cast<int>(ErrCode::OK);
                                (*reply)["message"] = "success";
                            }
                        });
                    }
                    return;
                }
                (*reply)["code"] = static_cast<int>(ErrCode::SYSTEM_ERROR);
                (*reply)["message"] = "system error";
            });
        });
    });
}

}

