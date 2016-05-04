#include "mysql_proxy.h"
#include "ini.hpp"
#include "exception.h"
#include "logging.h"

std::shared_ptr<MysqlProxy> get_local_sql()
{
    static thread_local std::shared_ptr<MysqlProxy> s_local_sql;
    if (!s_local_sql) {
        get_local_sql.reset(new MysqlProxy);
    }

    if (!s_local_sql->connected()) {
        s_local_sql->connect();
    }

    return s_local_sql;
}

MysqlProxy::MysqlProxy()
{
}

MysqlProxy::~MysqlProxy()
{
}

bool MysqlProxy::init(const std::string& conf_path)
{
    try {
        INI::Parser p(conf_path);
        _host = p.top()["host"];
        _user = p.top()["user"];
        _passwd = p.top()["password"];
        _db = p.top()["db"];
        _charset = p.top()["charset"];
        std::stringstream ss;
        ss << p.top()["port"];
        ss >> _port;
    } catch (std::runtime_error& e) {
        SLOG(WARNING) << e.what();
        return false;
    }

    mysql_library_init();
    return true;
}

void MysqlProxy::destroy()
{
    mysql_library_end();
}

void MysqlProxy::connect()
{
    if (!_my_sql) {
        auto p = mysql_init(nullptr);
        if (nullptr == p) {
            throw_system_error("insufficient memory");
        }

        _my_sql.reset(p, [](MYSQL* p) {
            if (nullptr != p) {
                mysql_close(p);
            }
        });
    } else {
        mysql_init(_my_sql.get());
    }
    
    mysql_options(_my_sql.get(), MYSQL_SET_CHARSET_NAME, _charset.c_str());
    if (nullptr == mysql_real_connect(_my_sql.get(),
                           _host.c_str(),
                           _user.c_str(),
                           _passwd.c_str(),
                           _db.c_str(),
                           _port,
                           NULL, //FIXME
                           0//CLIENT_MULTI_STATEMENTS
                          )) {
        SLOG(FATAL) << "Failed to connect to database: Error: " << mysql_error(p);
        throw_system_error("connect database fail");
    }
}

