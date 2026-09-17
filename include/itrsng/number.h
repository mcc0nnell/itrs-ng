#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ITRS_NUMBER_MAX_CANDIDATES 32
#define ITRS_NUMBER_ID_MAX 64
#define ITRS_NUMBER_URI_MAX 192
#define ITRS_NUMBER_TOKEN_MAX 64
#define ITRS_NUMBER_CSV_MAX 256

/** Candidate evidence flags. Positive and negative facts are both retained. */
typedef enum itrs_number_reason {
    ITRS_REASON_AVAILABLE = 1u << 0,
    ITRS_REASON_LANGUAGE_MATCH = 1u << 1,
    ITRS_REASON_MEDIA_MATCH = 1u << 2,
    ITRS_REASON_SERVICE_MATCH = 1u << 3,
    ITRS_REASON_JURISDICTION_MATCH = 1u << 4,
    ITRS_REASON_UNAVAILABLE = 1u << 16,
    ITRS_REASON_LANGUAGE_MISMATCH = 1u << 17,
    ITRS_REASON_MEDIA_MISMATCH = 1u << 18,
    ITRS_REASON_SERVICE_MISMATCH = 1u << 19,
    ITRS_REASON_JURISDICTION_MISMATCH = 1u << 20,
    ITRS_REASON_MALFORMED_RESOURCE = 1u << 21,
} itrs_number_reason_t;

typedef struct itrs_asl_resource {
    char id[ITRS_NUMBER_ID_MAX];
    char endpoint[ITRS_NUMBER_URI_MAX];
    char language[ITRS_NUMBER_TOKEN_MAX];
    char media[ITRS_NUMBER_CSV_MAX];
    char role[ITRS_NUMBER_TOKEN_MAX];
    char scope[ITRS_NUMBER_TOKEN_MAX];
    char jurisdictions[ITRS_NUMBER_CSV_MAX];
    char services[ITRS_NUMBER_CSV_MAX];
    char state[ITRS_NUMBER_TOKEN_MAX];
    char authority_source[ITRS_NUMBER_TOKEN_MAX];
    char authority_version[ITRS_NUMBER_TOKEN_MAX];
    int priority;
    bool dispatch_authority;
} itrs_asl_resource_t;

typedef struct itrs_number_request {
    char request_id[ITRS_NUMBER_ID_MAX];
    char service[ITRS_NUMBER_TOKEN_MAX];
    char language[ITRS_NUMBER_TOKEN_MAX];
    char media[ITRS_NUMBER_TOKEN_MAX];
    char jurisdiction_path[ITRS_NUMBER_CSV_MAX];
    char authoritative_psap_id[ITRS_NUMBER_ID_MAX];
    char authoritative_psap_endpoint[ITRS_NUMBER_URI_MAX];
    char resource_snapshot[ITRS_NUMBER_ID_MAX];
    char policy_version[ITRS_NUMBER_ID_MAX];
} itrs_number_request_t;

typedef struct itrs_number_candidate {
    char id[ITRS_NUMBER_ID_MAX];
    char endpoint[ITRS_NUMBER_URI_MAX];
    char role[ITRS_NUMBER_TOKEN_MAX];
    char scope[ITRS_NUMBER_TOKEN_MAX];
    char authority_source[ITRS_NUMBER_TOKEN_MAX];
    char authority_version[ITRS_NUMBER_TOKEN_MAX];
    bool eligible;
    int class_rank;
    int priority;
    uint32_t reason_mask;
} itrs_number_candidate_t;

typedef struct itrs_number_result {
    char request_id[ITRS_NUMBER_ID_MAX];
    char authoritative_psap_id[ITRS_NUMBER_ID_MAX];
    char authoritative_psap_endpoint[ITRS_NUMBER_URI_MAX];
    char resource_snapshot[ITRS_NUMBER_ID_MAX];
    char policy_version[ITRS_NUMBER_ID_MAX];
    bool selected;
    char selected_id[ITRS_NUMBER_ID_MAX];
    char selected_endpoint[ITRS_NUMBER_URI_MAX];
    char selected_role[ITRS_NUMBER_TOKEN_MAX];
    char selected_scope[ITRS_NUMBER_TOKEN_MAX];
    size_t candidate_count;
    itrs_number_candidate_t candidates[ITRS_NUMBER_MAX_CANDIDATES];
} itrs_number_result_t;

/**
 * Resolve an accessibility resource from an immutable snapshot.
 *
 * The authoritative PSAP fields are copied from request to result unchanged.
 * Resource ordering is deterministic and independent of input array order.
 * Returns 0 on success, EINVAL for invalid input, or EOVERFLOW when resource
 * count exceeds ITRS_NUMBER_MAX_CANDIDATES.
 */
int itrs_number_resolve(const itrs_number_request_t *request,
                        const itrs_asl_resource_t *resources,
                        size_t resource_count,
                        itrs_number_result_t *result);

/** Render a stable comma-separated explanation for a candidate reason mask. */
int itrs_number_format_reasons(uint32_t mask, char *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif
