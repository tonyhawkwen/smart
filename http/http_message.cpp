#include "http_message.h"
#include <unordered_map>
#include <sstream>
#include <cctype>
#include "logging.h"

namespace smart {

const std::string& http_reason(const HttpStatus& status)
{
    static std::unordered_map<int, std::string> reasons = {
        {(int)HttpStatus::CONTINUE, "Continue"},
        {(int)HttpStatus::OK, "OK"},
        {(int)HttpStatus::ACCEPTED, "Accepted"},
        {(int)HttpStatus::BAD_REQUEST, "Bad Reqeust"},
        {(int)HttpStatus::FORBIDDEN, "Forbidden"},
        {(int)HttpStatus::NOT_FOUND, "Not Found"},
        {(int)HttpStatus::TIMEOUT, "Request Timeout"},
        {(int)HttpStatus::LENGTH_REQUIRED, "Length Required"},
        {(int)HttpStatus::INTERNAL_SERVER_ERROR, "Internal Server Error"},
        {(int)HttpStatus::BAD_GATEWAY, "Bad Gateway"},
        {(int)HttpStatus::GATEWAY_TIMEOUT, "Gateway Timeout"},
        {(int)HttpStatus::VERSION_NOT_SUPPORTED, "HTTP Version Not Supported"}
    };

    return reasons[(int)status];
}

std::stringstream to_string(const struct http_parser& parser) {
    std::stringstream ss;
    ss << "\n[type:" << http_parser_type_name((http_parser_type)parser.type)
       << "]\n[state:" << http_parser_state_name(parser.state)
       << "]\n[header_state:" << http_parser_header_state_name(parser.header_state)
       << "]\n[http_errno:" << http_errno_description((http_errno)parser.http_errno)
       << "]\n[index:" << parser.index
       << "]\n[nread:" << parser.nread
       << "]\n[content_length:" << parser.content_length
       << "]\n[http_major:" << parser.http_major
       << "]\n[http_minor:" << parser.http_minor;
    if (parser.type == HTTP_RESPONSE || parser.type == HTTP_BOTH) {
        ss << "]\n[status_code:" << parser.status_code;
    }
    if (parser.type == HTTP_REQUEST || parser.type == HTTP_BOTH) {
        ss << "]\n[method:" << parser.method;
    }

    return ss;
}

http_parser_settings parser_settings = {
    &HttpMessage::on_message_begin,
    &HttpMessage::on_url,
    &HttpMessage::on_status,
    &HttpMessage::on_header_field,
    &HttpMessage::on_header_value,
    &HttpMessage::on_headers_complete,
    &HttpMessage::on_body,
    &HttpMessage::on_message_complete
};

HttpMessage::HttpMessage():
    _parsed_length(0),
    _stage(HTTP_ON_MESSAGE_BEGIN)
{
    http_parser_init(&_parser, HTTP_BOTH);
    _parser.data = this;
}

HttpMessage::~HttpMessage() {}

void HttpMessage::clear()
{
    http_parser_init(&_parser, HTTP_BOTH);
    _parser.data = this;
    _stage = HTTP_ON_MESSAGE_BEGIN;
    _parsed_length = 0;
}

ssize_t HttpMessage::parse(const Buffer& buf)
{
    auto datas = buf.strs();
    size_t nprocessed = 0;
    for (auto& data : datas) {
        if (data.second > 0) {
            nprocessed += http_parser_execute(&_parser, 
                    &parser_settings, (const char*)data.first, data.second);
            if (_parser.http_errno != 0) {
                SLOG(WARNING) << "fail to parse http message:" << to_string(_parser).str()
                              << "\nstage:" << _stage;
                return -1;
            }
            if (_stage == HTTP_ON_MESSAGE_COMPLELE) {
                break;
            }
        }   
    }
    _parsed_length += nprocessed;
    return nprocessed;
}

int HttpMessage::on_message_begin(http_parser* parser) {
    HttpMessage* http_message = static_cast<HttpMessage*>(parser->data);
    http_message->_stage = HTTP_ON_MESSAGE_BEGIN;
    return 0;
}

int HttpMessage::on_url(http_parser* parser, 
                        const char* at, const size_t length) {
    HttpMessage* http_message = static_cast<HttpMessage*>(parser->data);
    http_message->_stage = HTTP_ON_URL;
    http_message->_url.append(at, length);
    struct http_parser_url u;
    if(0 == http_parser_parse_url(http_message->_url.c_str(),
                http_message->_url.length(), 0, &u)) {
        if(u.field_set & (1 << UF_PATH) )  
        {  
            std::string path(
                    http_message->_url.c_str() + u.field_data[UF_PATH].off, 
                    u.field_data[UF_PATH].len);
            auto found = path.rfind('/');
            if (found != std::string::npos) {
                if (found + 1 < path.length()) {
                    http_message->_method_name = 
                        path.substr(found + 1, path.length() - found - 1);
                }

                if (found > 0) {
                    http_message->_service_name =
                        path.substr(1, found - 1);
                }
            }
        }  
    }

    return 0;
}

int HttpMessage::on_status(http_parser* parser, 
                           const char* at, const size_t length) {
    HttpMessage* http_message = static_cast<HttpMessage*>(parser->data);
    http_message->_stage = HTTP_ON_STATUS;
    http_message->_status.append(at, length);    
    return 0;
}

int HttpMessage::on_header_field(http_parser* parser,
                                 const char* at, const size_t length) {
    HttpMessage* http_message = static_cast<HttpMessage*>(parser->data);
    if (http_message->_stage != HTTP_ON_HEADER_FIELD) {
        http_message->_cur_header.clear();
        http_message->_stage = HTTP_ON_HEADER_FIELD;
    }
    http_message->_cur_header.append(at, length);
    std::for_each(http_message->_cur_header.begin(),
                  http_message->_cur_header.end(),
                  [](std::string::reference a){
                      a = tolower(a);
                 });
    return 0;
}

int HttpMessage::on_header_value(http_parser* parser,
                                 const char* at, const size_t length) {
    HttpMessage* http_message = static_cast<HttpMessage*>(parser->data);
    if (http_message->_cur_header.empty()) {
        return -1;
    }

    if (http_message->_stage == HTTP_ON_HEADER_FIELD) {
        http_message->_stage = HTTP_ON_HEADER_VALUE;
        http_message->_header_map.emplace(http_message->_cur_header, std::string(at, length));
    } else if (http_message->_stage == HTTP_ON_HEADER_VALUE) {
        auto itr = http_message->_header_map.find(http_message->_cur_header);
        if (itr != http_message->_header_map.end()) {
            itr->second.append(at, length);
        } else {
            return -1;
        } 
    } else {
        return -1;
    }

    return 0;
}

int HttpMessage::on_headers_complete(http_parser* parser) {
    HttpMessage* http_message = static_cast<HttpMessage*>(parser->data);
    http_message->_stage = HTTP_ON_HEADERS_COMPLELE;

    return 0;
}

int HttpMessage::on_body(http_parser* parser,
                         const char* at, const size_t length) {
    HttpMessage* http_message = static_cast<HttpMessage*>(parser->data);
    http_message->_stage = HTTP_ON_BODY;
    http_message->_content.append(at, length);
    return 0;
}

int HttpMessage::on_message_complete(http_parser* parser) {
    HttpMessage* http_message = static_cast<HttpMessage*>(parser->data);
    http_message->_stage = HTTP_ON_MESSAGE_COMPLELE;
    http_message->_cur_header.clear();

    return 0;
}

bool HttpMessage::set_response(Buffer& buff, const HttpStatus& status, const std::string& content)
{
    char data[64];
    int len = snprintf(data, sizeof(data), "HTTP/%d.%d %d ",
                       _parser.http_major,
                       _parser.http_minor,
                       (int32_t)status);
    buff.append((unsigned char*)data, len);
    buff.append(http_reason(status));
    buff.append("\r\n");

    len = snprintf(data, sizeof(data), "content-length: %lu\r\n",
                       content.length());
    buff.append((unsigned char*)data, len);
    buff.append("content-type: application/json\r\n");

    for (auto& header : _header_map) {
        if (header.first == "content-length"
                || header.first == "content-type") {
            continue;
        }
        buff.append(header.first);
        buff.append(": ");
        buff.append(header.second);
        buff.append("\r\n");
    }
    buff.append("\r\n");
    buff.append(content);

    return true;
}

}

