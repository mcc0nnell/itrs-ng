#pragma once

#include "itrsng/number.h"

#define ITRS_NUMBER_SERVICE_NAME "org.itrsng.number"
#define ITRS_NUMBER_SERVICE_VERSION "1.0.0"

typedef struct itrs_number_service {
    void *handle;
    int (*resolve)(void *handle, const itrs_number_request_t *request, itrs_number_result_t *result);
} itrs_number_service_t;
