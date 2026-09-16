#include "itrs_access_identity_service.h"

#include <celix_bundle_activator.h>
#include <stdio.h>
#include <string.h>

typedef struct activator_data {
    long service_id;
    itrs_access_identity_service_t service;
} activator_data_t;

static void set_text(char *dst, size_t size, const char *value) {
    snprintf(dst, size, "%s", value);
}

static int snapshot(void *handle, itrs_access_context_t *context) {
    (void)handle;
    if (!context) return 22;
    memset(context, 0, sizeof(*context));
    set_text(context->observation_id, sizeof(context->observation_id), "celix-access-roaming-001");
    set_text(context->observed_at, sizeof(context->observed_at), "2026-09-16T21:45:00Z");
    set_text(context->source_kind, sizeof(context->source_kind), "synthetic");
    set_text(context->source_authority, sizeof(context->source_authority), "windanvil-lab");
    set_text(context->transport, sizeof(context->transport), "cellular");
    set_text(context->profile_state, sizeof(context->profile_state), "active");
    set_text(context->attachment, sizeof(context->attachment), "roaming");
    set_text(context->ims_state, sizeof(context->ims_state), "registered");
    set_text(context->emergency_access_state, sizeof(context->emergency_access_state), "unknown");
    context->capability_count = 2;
    set_text(context->capabilities[0].name, sizeof(context->capabilities[0].name), "voice");
    set_text(context->capabilities[0].state, sizeof(context->capabilities[0].state), "available");
    set_text(context->capabilities[0].basis, sizeof(context->capabilities[0].basis), "synthetic");
    set_text(context->capabilities[1].name, sizeof(context->capabilities[1].name), "rtt");
    set_text(context->capabilities[1].state, sizeof(context->capabilities[1].state), "available");
    set_text(context->capabilities[1].basis, sizeof(context->capabilities[1].basis), "synthetic");
    return 0;
}

static celix_status_t activator_start(activator_data_t *data, celix_bundle_context_t *ctx) {
    data->service.handle = data;
    data->service.snapshot = snapshot;
    data->service_id = celix_bundleContext_registerService(ctx, &data->service, ITRS_ACCESS_IDENTITY_SERVICE_NAME, NULL);
    return data->service_id < 0 ? CELIX_BUNDLE_EXCEPTION : CELIX_SUCCESS;
}

static celix_status_t activator_stop(activator_data_t *data, celix_bundle_context_t *ctx) {
    if (data->service_id >= 0) celix_bundleContext_unregisterService(ctx, data->service_id);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(activator_data_t, activator_start, activator_stop)
