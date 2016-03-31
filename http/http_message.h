#ifndef SERVICE_HTTP_MESSAGE_H
#define SERVICE_HTTP_MESSAGE_H

#include <map>
#include <memory>
#include "http_parser.h"
#include "buffer.h"

namespace smart {

enum class HttpStatus {
    CONTINUE = 100,
    OK = 200,
    ACCEPTED = 202,
    BAD_REQUEST = 400,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    TIMEOUT = 408,
    LENGTH_REQUIRED = 411,
    INTERNAL_SERVER_ERROR = 500,
    BAD_GATEWAY = 502,
    GATEWAY_TIMEOUT = 504,
    VERSION_NOT_SUPPORTED = 505
};

class HttpMessage {
public:
    friend class HttpMessageResponse;
    enum HttpParserStage {
        HTTP_ON_MESSAGE_BEGIN,
        HTTP_ON_URL,
        HTTP_ON_STATUS,
        HTTP_ON_HEADER_FIELD, 
        HTTP_ON_HEADER_VALUE,
        HTTP_ON_HEADERS_COMPLELE,
        HTTP_ON_BODY,
        HTTP_ON_MESSAGE_COMPLELE
    };

    HttpMessage();
    ~HttpMessage();

    ssize_t parse(const Buffer& buf);
    bool completed() const { return _stage == HTTP_ON_MESSAGE_COMPLELE; }
    void clear();
    const std::string& content() { return _content; }
    bool set_response(Buffer& buff, const HttpStatus& status, const std::string& content);
    const std::string& get_service_name() { return _service_name; }
    const std::string& get_method_name() { return _method_name; }
// Http parser callback functions
    static int on_message_begin(http_parser *);
    static int on_url(http_parser *, const char *, const size_t);
    static int on_status(http_parser*, const char *, const size_t);
    static int on_header_field(http_parser *, const char *, const size_t);
    static int on_header_value(http_parser *, const char *, const size_t);
    static int on_headers_complete(http_parser *);
    static int on_body(http_parser *, const char *, const size_t);
    static int on_message_complete(http_parser *);
    
    friend inline std::ostream& operator<< (std::ostream& os, const HttpMessage& msg)
    {
        os << "\nparsed length:" << msg._parsed_length
           << "\nstage:" << msg._stage
           << "\nurl:" << msg._url
           << "\nstatus:" << msg._status
           << "\nheaders:";
        for (auto& itr : msg._header_map) {
            os << "\n\t[" << itr.first << ":" << itr.second <<"]";
        }
        os << "\ncontent:" << msg._content;

        return os;
    }

private:
    size_t _parsed_length;
    HttpParserStage _stage;
    struct http_parser _parser;
    std::string _url;
    std::string _service_name;
    std::string _method_name;
    std::string _status;
    std::string _cur_header;
    std::map<std::string, std::string> _header_map;
    std::string _content;
    HttpStatus _status_code;
};

using SHttpMessage = std::shared_ptr<HttpMessage>;

}


#endif /*SERVICE_HTTP_MESSAGE_H*/
