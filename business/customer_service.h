#ifndef BUSINESS_CUSTOMER_SERVICE_H
#define BUSINESS_CUSTOMER_SERVICE_H

#include "service.h"

namespace smart {

class CustomerService : public Service {
public:
    CustomerService() = default;
    ~CustomerService() {}

    void init() override;
};

}

#endif /*BUSINESS_CUSTOMER_SERVICE_H*/
