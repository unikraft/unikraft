#ifndef SERVICE_EXPORT_H
#define SERVICE_EXPORT_H
#include "aeerror.h"

struct IService
{
    virtual ~IService() = default;
    virtual ae_error_t start() = 0;
    virtual void stop() = 0;
};

#endif /* SERVICE_EXPORT_H */
