#ifndef MYSQL_MYSQL_PROXY_H
#define MYSQL_MYSQL_PROXY_H

#include <string>
#include <memory>
#include <mysql.h>

class MysqlProxy {
public:
    MysqlProxy();
    ~MysqlProxy();

    bool connected() {
        return _my_sql && _my_sql->mysql_ping() == 0;
    }
    void connect();
    bool init(const std::string& conf_path);
    void destroy();

private:
    std::shared_ptr<MYSQL> _my_sql;
    std::string _host;
    std::string _user;
    std::string _passwd;
    std::string _db;
    std::string _charset;
    unsigned int _port;
};

std::shared_ptr<MysqlProxy> get_local_sql();

#endif /*MYSQL_MYSQL_PROXY_H*/
