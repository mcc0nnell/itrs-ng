#pragma once

#include "itrsng/access.h"

#define ITRS_ACCESS_IDENTITY_SERVICE_NAME "org.itrsng.access.identity"
#define ITRS_ACCESS_IDENTITY_SERVICE_VERSION "1.0.0"

typedef struct itrs_access_identity_service {
    void *handle;
    int (*snapshot)(void *handle, itrs_access_context_t *context);
} itrs_access_identity_service_t;
