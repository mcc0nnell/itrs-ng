#pragma once

#include "itrs_enum_provider_service.h"
#include "itrsng/number.h"

#define ITRS_NUMBER_SERVICE_NAME "org.itrsng.number"
#define ITRS_NUMBER_SERVICE_VERSION "1.1.0"

typedef struct itrs_number_enum_result {
    itrs_enum_result_t resolution;
    char provider_source[ITRS_ENUM_PROVIDER_SOURCE_MAX];
    uint64_t observed_at_unix_ms;
    uint32_t min_ttl_seconds;
    bool authenticated_data;
    size_t observed_record_count;
} itrs_number_enum_result_t;

typedef struct itrs_number_service {
    void *handle;
    int (*resolve)(void *handle, const itrs_number_request_t *request,
                   itrs_number_result_t *result);
    int (*resolveE164)(void *handle, const char *e164, const char *apex,
                       itrs_number_enum_result_t *result);
} itrs_number_service_t;
