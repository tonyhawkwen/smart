#ifndef PUBLIC_EXCEPTION_H
#define PUBLIC_EXCEPTION_H
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <cstring>
#include <cerrno>

void throw_system_error(const std::string&);
inline void throw_system_error(const std::string& msg) {
    throw std::system_error(errno, std::system_category(), msg);
}

void throw_system_error(int err, const std::string&);
inline void throw_system_error(int err, const std::string& msg) {
    throw std::system_error(err, std::system_category(), msg);
}

class Exception : public std::exception {
 public:
    explicit Exception(const std::string& value) : _value(value) {}
    explicit Exception(const std::string&& value) : _value(std::move(value)) {}
    virtual ~Exception(void) throw() {}

    virtual const char *what(void) const throw() {
        return _value.c_str();
    }

private:
    std::string _value;
};


class RunException : public std::runtime_error {
public:
    enum ExceptionType {
        UNKNOWN = 0,
    };

    RunException(const std::string& message) : 
        std::runtime_error(get_message(message, UNKNOWN, 0)), 
        _type(UNKNOWN), _errno(0){}

    RunException(ExceptionType type, const std::string& message) : 
        std::runtime_error(get_message(message, type, 0)), 
        _type(type), _errno(0){}

    RunException(ExceptionType type, const std::string& message, int err) : 
        std::runtime_error(get_message(message, type, err)),
        _type(type), _errno(err){}

    ExceptionType get_type() const noexcept { return _type; }
    int get_errno() const noexcept { return _errno; }

protected:
    std::string get_message(const std::string &message, ExceptionType type, int err)
    {
        std::stringstream ss;
        ss << "Exception [" << message << "]type[" << (int)type << "]errno[" << err << "]";
        if (err != 0)
        {
            char buffer[256];
            ss <<"errinfo[" << strerror_r(err, buffer, 256) << "]";
        }

        return ss.str();
    }

private:
    ExceptionType _type;
    int _errno;
};

#endif


