#ifndef SERVICE_AUTH_SERVICE_H
#define SERVICE_AUTH_SERVICE_H

#include "service.h"

namespace smart {

class AccessService : public Service {
public:
    AccessService() = default;
    ~AccessService() {}

    void init() override;
};

}

#endif /*SERVICE_AUTH_SERVICE_H*/
