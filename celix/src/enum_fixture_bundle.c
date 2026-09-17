#include "itrs_enum_provider_service.h"

#include <celix_bundle_activator.h>
#include <celix_constants.h>
#include <celix_properties.h>

#include <errno.h>
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

static int lookup_fixture(void *handle, const char *e164, const char *apex,
                          itrs_enum_observation_t *observation) {
    (void)handle;
    if (e164 == NULL || observation == NULL) return EINVAL;
    const char *effective_apex = (apex == NULL || apex[0] == '\0') ? ITRS_ENUM_DEFAULT_APEX : apex;
    memset(observation, 0, sizeof(*observation));
    int rc = itrs_enum_domain_from_e164(e164, effective_apex,
                                        observation->query_domain,
                                        sizeof(observation->query_domain));
    if (rc != 0) return rc;
    snprintf(observation->source, sizeof(observation->source), "celix-enum-fixture");
    observation->observed_at_unix_ms = now_ms();
    observation->min_ttl_seconds = 60;
    observation->authenticated_data = false;
    observation->record_count = 1;
    itrs_naptr_record_t *r = &observation->records[0];
    r->order = 10;
    r->preference = 11;
    snprintf(r->flags, sizeof(r->flags), "u");
    snprintf(r->service, sizeof(r->service), "E2U+sip");
    snprintf(r->regexp, sizeof(r->regexp), "!^(.*)$!sip:\\1@providerGW.example.com!");
    snprintf(r->replacement, sizeof(r->replacement), ".");
    return 0;
}

static celix_status_t activator_start(activator_data_t *data, celix_bundle_context_t *ctx) {
    data->service.handle = data;
    data->service.lookup = lookup_fixture;
    celix_properties_t *props = celix_properties_create();
    celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_RANKING, 1000L);
    celix_properties_set(props, "itrs.enum.provider.kind", "fixture");
    data->service_id = celix_bundleContext_registerService(ctx, &data->service,
                                                           ITRS_ENUM_PROVIDER_SERVICE_NAME, props);
    return data->service_id < 0 ? CELIX_BUNDLE_EXCEPTION : CELIX_SUCCESS;
}

static celix_status_t activator_stop(activator_data_t *data, celix_bundle_context_t *ctx) {
    if (data->service_id >= 0) celix_bundleContext_unregisterService(ctx, data->service_id);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(activator_data_t, activator_start, activator_stop)
