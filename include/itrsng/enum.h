#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ITRS_ENUM_DEFAULT_APEX "itrs.us"
#define ITRS_ENUM_DOMAIN_MAX 256
#define ITRS_ENUM_URI_MAX 256
#define ITRS_ENUM_FLAGS_MAX 8
#define ITRS_ENUM_SERVICE_MAX 128
#define ITRS_ENUM_REGEXP_MAX 256
#define ITRS_ENUM_REPLACEMENT_MAX 256
#define ITRS_ENUM_MAX_RECORDS 32

typedef struct itrs_naptr_record {
    uint16_t order;
    uint16_t preference;
    char flags[ITRS_ENUM_FLAGS_MAX];
    char service[ITRS_ENUM_SERVICE_MAX];
    char regexp[ITRS_ENUM_REGEXP_MAX];
    char replacement[ITRS_ENUM_REPLACEMENT_MAX];
} itrs_naptr_record_t;

typedef struct itrs_enum_result {
    char aus[32];
    char query_domain[ITRS_ENUM_DOMAIN_MAX];
    char uri[ITRS_ENUM_URI_MAX];
    uint16_t order;
    uint16_t preference;
    size_t source_index;
} itrs_enum_result_t;

/** Normalize a fully qualified E.164 number to the ENUM Application Unique String. */
int itrs_enum_normalize_e164(const char *input, char *aus, size_t aus_size);

/** Build the reversed-digit ENUM query name beneath the supplied apex. */
int itrs_enum_domain_from_e164(const char *input, const char *apex,
                               char *domain, size_t domain_size);

/**
 * Select and apply the first deterministic terminal E2U+sip NAPTR rule.
 * Records are processed by order, preference, then canonical record content.
 * Unknown/unsupported records are skipped. Returns ENOENT when none apply.
 */
int itrs_enum_resolve_sip(const char *input, const char *apex,
                          const itrs_naptr_record_t *records, size_t record_count,
                          itrs_enum_result_t *result);

#ifdef __cplusplus
}
#endif
