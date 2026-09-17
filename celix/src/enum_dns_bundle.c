#include "itrs_enum_provider_service.h"

#include <celix_bundle_activator.h>
#include <celix_constants.h>
#include <celix_properties.h>

#include <arpa/nameser.h>
#include <errno.h>
#include <netdb.h>
#include <resolv.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct activator_data {
    itrs_enum_provider_service_t service;
    long service_id;
} activator_data_t;

static uint64_t now_ms(void) {
    struct timespec ts = {0};
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
}

static int copy_char_string(const unsigned char **cursor, const unsigned char *end,
                            char *out, size_t out_size) {
    if (cursor == NULL || *cursor == NULL || *cursor >= end || out == NULL || out_size == 0) return EINVAL;
    size_t len = *(*cursor)++;
    if ((size_t)(end - *cursor) < len || len + 1 > out_size) return EMSGSIZE;
    memcpy(out, *cursor, len);
    out[len] = '\0';
    *cursor += len;
    return 0;
}

static int parse_naptr(const unsigned char *message, size_t message_len, const ns_rr *rr,
                       itrs_naptr_record_t *out) {
    const unsigned char *rdata = ns_rr_rdata(*rr);
    const unsigned char *end = rdata + ns_rr_rdlen(*rr);
    if ((size_t)(end - rdata) < 4) return EINVAL;
    memset(out, 0, sizeof(*out));
    out->order = ns_get16(rdata);
    out->preference = ns_get16(rdata + 2);
    const unsigned char *cursor = rdata + 4;
    int rc = copy_char_string(&cursor, end, out->flags, sizeof(out->flags));
    if (rc != 0) return rc;
    rc = copy_char_string(&cursor, end, out->service, sizeof(out->service));
    if (rc != 0) return rc;
    rc = copy_char_string(&cursor, end, out->regexp, sizeof(out->regexp));
    if (rc != 0) return rc;
    if (cursor > end) return EINVAL;
    int expanded = dn_expand(message, message + message_len, cursor,
                             out->replacement, sizeof(out->replacement));
    if (expanded < 0) return EINVAL;
    if (out->replacement[0] == '\0') snprintf(out->replacement, sizeof(out->replacement), ".");
    return 0;
}

static int map_resolver_error(const struct __res_state *state) {
    switch (state->res_h_errno) {
        case HOST_NOT_FOUND:
#ifdef NO_DATA
        case NO_DATA:
#endif
            return ENOENT;
        case TRY_AGAIN:
            return EAGAIN;
        default:
            return EIO;
    }
}

static int lookup_dns(void *handle, const char *e164, const char *apex,
                      itrs_enum_observation_t *observation) {
    (void)handle;
    if (e164 == NULL || observation == NULL) return EINVAL;
    const char *effective_apex = (apex == NULL || apex[0] == '\0') ? ITRS_ENUM_DEFAULT_APEX : apex;
    memset(observation, 0, sizeof(*observation));
    int rc = itrs_enum_domain_from_e164(e164, effective_apex,
                                        observation->query_domain,
                                        sizeof(observation->query_domain));
    if (rc != 0) return rc;

    struct __res_state state;
    memset(&state, 0, sizeof(state));
    if (res_ninit(&state) != 0) return EIO;
    unsigned char answer[8192];
    int answer_len = res_nquery(&state, observation->query_domain,
                                ns_c_in, ns_t_naptr, answer, sizeof(answer));
    if (answer_len < 0) {
        rc = map_resolver_error(&state);
        res_nclose(&state);
        return rc;
    }

    ns_msg msg;
    if (ns_initparse(answer, answer_len, &msg) < 0) {
        res_nclose(&state);
        return EPROTO;
    }
    snprintf(observation->source, sizeof(observation->source), "celix-system-dns");
    observation->observed_at_unix_ms = now_ms();
    observation->authenticated_data = ns_msg_getflag(msg, ns_f_ad) != 0;
    uint32_t min_ttl = UINT32_MAX;
    const int answers = ns_msg_count(msg, ns_s_an);
    for (int i = 0; i < answers; ++i) {
        ns_rr rr;
        if (ns_parserr(&msg, ns_s_an, i, &rr) < 0) {
            res_nclose(&state);
            return EPROTO;
        }
        if (ns_rr_type(rr) != ns_t_naptr) continue;
        if (observation->record_count >= ITRS_ENUM_MAX_RECORDS) {
            res_nclose(&state);
            return EOVERFLOW;
        }
        rc = parse_naptr(answer, (size_t)answer_len, &rr,
                         &observation->records[observation->record_count]);
        if (rc != 0) {
            res_nclose(&state);
            return rc;
        }
        uint32_t ttl = ns_rr_ttl(rr);
        if (ttl < min_ttl) min_ttl = ttl;
        observation->record_count++;
    }
    res_nclose(&state);
    if (observation->record_count == 0) return ENOENT;
    observation->min_ttl_seconds = min_ttl == UINT32_MAX ? 0 : min_ttl;
    return 0;
}

static celix_status_t activator_start(activator_data_t *data, celix_bundle_context_t *ctx) {
    data->service.handle = data;
    data->service.lookup = lookup_dns;
    celix_properties_t *props = celix_properties_create();
    celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_RANKING, 100L);
    celix_properties_set(props, "itrs.enum.provider.kind", "system-dns");
    data->service_id = celix_bundleContext_registerService(ctx, &data->service,
                                                           ITRS_ENUM_PROVIDER_SERVICE_NAME, props);
    return data->service_id < 0 ? CELIX_BUNDLE_EXCEPTION : CELIX_SUCCESS;
}

static celix_status_t activator_stop(activator_data_t *data, celix_bundle_context_t *ctx) {
    if (data->service_id >= 0) celix_bundleContext_unregisterService(ctx, data->service_id);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(activator_data_t, activator_start, activator_stop)
