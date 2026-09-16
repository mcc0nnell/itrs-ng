#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ITRS_ACCESS_ID_MAX 64
#define ITRS_ACCESS_TIME_MAX 40
#define ITRS_ACCESS_TOKEN_MAX 64
#define ITRS_ACCESS_MAX_CAPABILITIES 8

typedef struct itrs_access_capability {
    char name[ITRS_ACCESS_TOKEN_MAX];
    char state[ITRS_ACCESS_TOKEN_MAX];
    char basis[ITRS_ACCESS_TOKEN_MAX];
} itrs_access_capability_t;

typedef struct itrs_access_context {
    char observation_id[ITRS_ACCESS_ID_MAX];
    char observed_at[ITRS_ACCESS_TIME_MAX];
    char source_kind[ITRS_ACCESS_TOKEN_MAX];
    char source_authority[ITRS_ACCESS_TOKEN_MAX];
    char transport[ITRS_ACCESS_TOKEN_MAX];
    char profile_state[ITRS_ACCESS_TOKEN_MAX];
    char attachment[ITRS_ACCESS_TOKEN_MAX];
    char ims_state[ITRS_ACCESS_TOKEN_MAX];
    char emergency_access_state[ITRS_ACCESS_TOKEN_MAX];
    size_t capability_count;
    itrs_access_capability_t capabilities[ITRS_ACCESS_MAX_CAPABILITIES];
} itrs_access_context_t;

/**
 * Validate the normalized access-context@1 shape used by the reference runtime.
 *
 * Invalid or missing access context is non-fatal to Number resolution. This
 * helper exists so consumers can distinguish usable access evidence from an
 * absent or malformed optional observation.
 */
bool itrs_access_context_is_valid(const itrs_access_context_t *context);

#ifdef __cplusplus
}
#endif
