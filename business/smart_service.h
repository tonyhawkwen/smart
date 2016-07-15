#ifndef BUSINESS_SMART_SERVICE_H
#define BUSINESS_SMART_SERVICE_H

#include <vector>
#include "service_data.h"

namespace smart {

struct SmartProtocal {
    unsigned char head[2];
    unsigned char type;
    unsigned char length;
    unsigned char mac[8];
    unsigned char short_addr[2];
    unsigned char long_addr[8];
    unsigned char device_type;
};

struct TLV {
    size_t type;
    size_t length;
    size_t value;
    TLV()= default;
    ~TLV() = default;
    TLV(size_t t, size_t l, size_t v) : type(t), length(l), value(v) {}

    friend inline std::ostream& operator<< (std::ostream& os, const TLV& buf)
    {
        os << "type:" << buf.type << " length:" << buf.length << " value:" << buf.value;
        return os;
    }
};

class SmartService {
public:
    SmartService() = default;
    ~SmartService() {}
    void parse(SConnection& conn);

};

}

#endif /*BUSINESS_SMART_SERVICE_H*/
