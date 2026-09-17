#pragma once

#include "itrsng/enum.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ITRS_ENUM_PROVIDER_SERVICE_NAME "org.itrsng.enum.provider"
#define ITRS_ENUM_PROVIDER_SERVICE_VERSION "1.0.0"
#define ITRS_ENUM_PROVIDER_SOURCE_MAX 64

typedef struct itrs_enum_observation {
    char source[ITRS_ENUM_PROVIDER_SOURCE_MAX];
    char query_domain[ITRS_ENUM_DOMAIN_MAX];
    uint64_t observed_at_unix_ms;
    uint32_t min_ttl_seconds;
    bool authenticated_data;
    size_t record_count;
    itrs_naptr_record_t records[ITRS_ENUM_MAX_RECORDS];
} itrs_enum_observation_t;

typedef struct itrs_enum_provider_service {
    void *handle;
    int (*lookup)(void *handle, const char *e164, const char *apex,
                  itrs_enum_observation_t *observation);
} itrs_enum_provider_service_t;
